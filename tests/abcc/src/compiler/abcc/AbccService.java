package compiler.abcc;

import android.app.IntentService;
import android.app.Service;
import android.content.Intent;
import android.content.res.AssetManager;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.os.Binder;
import android.os.Bundle;
import android.os.IBinder;
import android.os.Parcel;
import android.os.ResultReceiver;
import android.os.RemoteException;
import android.util.Log;
import java.io.File;
import java.io.FilenameFilter;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

public class AbccService extends IntentService {
  private static final String TAG = "AbccService";

  String mWorkingDir = null;
  String mSysroot = null;

  // For onBind()
  private IBinder mBinder = new LocalBinder();
  private int mCurrentStatus = AbccService.STATUS_OKAY;
  private Object mStatusLock = new Object();

  private static int STATUS_OKAY = 0;
  private static int STATUS_WORKING = 1;
  private static int STATUS_ERROR = 2;

  class WorkingThread extends Thread {
    @Override
    public void run() {
      Log.i(TAG, "WorkingThread run");


      synchronized (mStatusLock) {
        mCurrentStatus = AbccService.STATUS_WORKING;
        if (mWorkingDir == null) {
          mCurrentStatus = AbccService.STATUS_ERROR;
          mStatusLock.notifyAll();
          return;
        }
      }

      Log.i(TAG, "mWorkingDir for genLibs: " + mWorkingDir);
      if (genLibs(mWorkingDir, mSysroot) != 0) {
        synchronized (mStatusLock) {
          mCurrentStatus = AbccService.STATUS_ERROR;
          mStatusLock.notifyAll();
          return;
        }
      }

      Log.i(TAG, "WorkingThread run okay");
      synchronized (mStatusLock) {
        mCurrentStatus = AbccService.STATUS_OKAY;
        mStatusLock.notifyAll();
        Log.i(TAG, "WorkingThread run done");
        return;
      }
    }
  }

  class LocalBinder extends Binder {
    @Override
    protected boolean onTransact(int code, Parcel data, Parcel reply, int flags) throws RemoteException {
      if (!pingBinder()) {
        Log.e(TAG, "Process doesn't exist. Error!");
        throw new RemoteException("AbccLocalBinder pingBinder return false!");
      }

      // The code is not important now. This is just a communication way
      // and what we really need is the information from @data
      Bundle real_data = data.readBundle();
      if (real_data == null) {
        Log.e(TAG, "Should get some information from the Parcel.");
        return false;
      }
      String path = real_data.getString("working_dir");
      if (path == null ) {
        Log.e(TAG, "onTransact but no working_dir provided?");
        return false;
      }

      mWorkingDir = path;
      compileForBinder();
      return queryStatusForBinder();
    }
  }

  private boolean compileForBinder() {
    Log.i(TAG, "compileForBinder");
    new WorkingThread().start();
    synchronized (mStatusLock) {
      mCurrentStatus = AbccService.STATUS_WORKING;
    }
    Log.i(TAG, "compileForBinder done");
    return true;
  }

  private boolean queryStatusForBinder() {
    Log.i(TAG, "queryStatusForBinder");
    Log.i(TAG, "mCurrentStatus = " + mCurrentStatus);

    while (mCurrentStatus == STATUS_WORKING) {
      try {
        Log.i(TAG, "queryStatusForBinder wait");
        synchronized (mStatusLock) {
          mStatusLock.wait();
        }
      } catch (InterruptedException e) {
      }
    }

    Log.i(TAG, "queryStatusForBinder checking result");

    if (mCurrentStatus == STATUS_OKAY)
      return true;
    else
      return false;
  }

  private void extractIntentInfo(Intent intent) {
    if (intent == null) {
      Log.e(TAG, "Intent should not be null when extractIntentInfo.");
      return;
    }

    String path = intent.getStringExtra("working_dir");
    Log.i(TAG, "Got working_dir from intent: " + path);
    if (path != null)
      mWorkingDir = path;
    else
      Log.e(TAG, "Intent extra 'working_dir' cannot be null.");

    path = intent.getStringExtra("toolchain_sysroot");
    if (path != null)
      mSysroot = path;
  }

  private void installToolchain() {
    if (mSysroot != null) {
      // User specify a customized toolchain sysroot, so we don't need to copy.
      return;
    }

    // The toolchain is enclosed inside assets/
    File sysroot = getDir("toolchain_sysroot", MODE_WORLD_READABLE);
    mSysroot = sysroot.getAbsolutePath();
    File cur_file = new File(mSysroot + "/usr");
    cur_file.mkdirs();
    cur_file.setReadable(true, /*OwnerOnly=*/false);
    cur_file.setExecutable(true, /*OwnerOnly=*/false);
    cur_file = new File(mSysroot + "/usr/bin");
    cur_file.mkdirs();
    cur_file.setReadable(true, /*OwnerOnly=*/false);
    cur_file.setExecutable(true, /*OwnerOnly=*/false);
    cur_file = new File(mSysroot + "/usr/lib");
    cur_file.mkdirs();
    cur_file.setReadable(true, /*OwnerOnly=*/false);
    cur_file.setExecutable(true, /*OwnerOnly=*/false);
    copyAssets("usr/bin", mSysroot + "/usr/bin", /*executable=*/true);
    copyAssets("usr/lib", mSysroot + "/usr/lib", /*executable=*/false);
  }

  private void copyAssets(String asset_dir, String out_dir, boolean executable) {
    AssetManager assetManager = getAssets();
    String[] files = null;
    try {
      files = assetManager.list(asset_dir);
    } catch (IOException e) {
      Log.e(TAG, "Failed to get asset file list.", e);
    }
    for(String filename : files) {
      InputStream in = null;
      OutputStream out = null;
      try {
        in = assetManager.open(asset_dir + "/" + filename);
        out = new FileOutputStream(out_dir + "/" + filename);
        copyFile(in, out);
        in.close();
        in = null;
        out.flush();
        out.close();
        out = null;
      } catch(IOException e) {
        Log.e(TAG, "Failed to copy asset file: " + filename, e);
      }

      File cur_file = new File(out_dir + "/" + filename);
      cur_file.setReadable(true, /*OwnerOnly=*/false);
      if (executable) {
        cur_file.setExecutable(true, /*OwnerOnly=*/false);
      }
    }
  }

  private void copyFile(InputStream in, OutputStream out) throws IOException {
    byte[] buffer = new byte[1024];
    int read;
    while((read = in.read(buffer)) != -1){
      out.write(buffer, 0, read);
    }
  }

  public AbccService() {
    super("AbccService");
  }

  // Usage: adb shell am startservice -a compiler.abcc.BITCODE_COMPILE_TEST -n compiler.abcc/compiler.abcc.AbccService -e working_dir /data/local/tmp/test
  @Override
  public void onHandleIntent(Intent intent) {
    Log.i(TAG, "got onHandleIntent intent: " + intent);
    if (intent.getAction() != "compiler.abcc.BITCODE_COMPILE_TEST") {
      Log.e(TAG, "We don't support formal release by onHandleIntent() yet!");
      return;
    }

    extractIntentInfo(intent);
    installToolchain();
    new WorkingThread().start();
  }

  @Override
  public IBinder onBind(Intent intent) {
    Log.i(TAG, "got onBind intent: " + intent);
    if (intent.getAction() != "compiler.abcc.BITCODE_COMPILE") {
      Log.e(TAG, "We don't support other intent except for BITCODE_COMPILE by onBind() yet!");
      return null;
    }

    installToolchain();

    if (mBinder == null) {
      Log.e(TAG, "Why mBinder is null?");
    }
    return mBinder;
  }

  private void dumpDebugInfo() {
    Log.i(TAG, "AbccService field dump:");
    Log.i(TAG, "- mWorkingDir: " + mWorkingDir);
    Log.i(TAG, "- mSysroot: " + mSysroot);
    Log.i(TAG, "- mBinder: " + mBinder);
    Log.i(TAG, "- mCurrentStatus: " + mCurrentStatus);
  }

  @Override
  public boolean onUnbind(Intent intent) {
    Log.i(TAG, "got onUnbind intent: " + intent);
    return false;
  }

  // If succeess, it will be 0 in file working_dir/compile_result.
  // Otherwise, there will be error message in file working_dir/compile_error.
  private native int genLibs(String working_dir, String sysroot);

  static {
    // Distinguish whether this is a system prebuilt apk or an updated apk
    // ApplicationInfo info = getApplicationInfo();
    // FIXME: We cannot access ApplicationInfo in static context, how to prevent magic path?
    if (new File("/data/data/compiler.abcc/lib/libjni_abcc.so").exists())
      System.loadLibrary("jni_abcc");
    else
      System.load("/system/lib/libjni_abcc.so");
  }
}
