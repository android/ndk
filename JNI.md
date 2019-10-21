# JNI

The Java Native Interface (JNI) is how you call C/C++ ("native") code from Java/Kotlin ("managed") code, or the other way around.

There's a full [Java Native Interface Specification](https://docs.oracle.com/en/java/javase/13/docs/specs/jni/index.html), but this document should give you enough detail to write useful code without being overwhelmed.




## "Hello, World!" in JNI

The "HelloJni" sample in Android Studio shows the basics of calling C/C++ from Java.

1. **Load the C/C++ code into your app**. The [`System.loadLibrary`](https://developer.android.com/reference/java/lang/System.html#loadLibrary(java.lang.String)) method loads C/C++ code. If your library has the filename `libfoo.so`, you would pass just `"foo"` because `System.loadLibrary` itself will add the appropriate prefix and suffix for the OS you're running on.

```java
  static {
    System.loadLibrary("foo");
  }
```

2. **Declare your native methods**. You can use the `native` attribute in Java (or `external` in Kotlin) to tell the compiler that your method is actually implemented in C/C++.

```java
  public native void foo();
```
3. **Define your native methods**. The `<jni.h>` header contains everything you need to write the necessary boilerplate.

```c++
#include <jni.h>

extern "C" JNIEXPORT void JNICALL Java_com_example_app_Main_foo(JNIEnv* env, jobject o) {
  // Do stuff here!
}
```

Now you can invoke `foo()` in Java/Kotlin code just like a normal method, and your C/C++ implementation will be called.


## Types in <jni.h>

### Primitive types

The `<jni.h>` header defines C/C++ types corresponding to the major types in Java. The primitive types map quite simply:

| C/C++    | Java    |
| -------- | ------- |
| jboolean | boolean |
| jbyte    | byte    |
| jchar    | char    |
| jdouble  | double  |
| jfloat   | float   |
| jint     | int     |
| jlong    | long    |
| jshort   | short   |

### Reference types

There are also types for a few of the more important reference types:

| C/C++       | Java                 |
| ----------- | -------------------- |
| jclass      | java.lang.Class      |
| jobject     | java.lang.Object     |
| jstring     | java.lang.String     |
| jthrowable  | java.lang.Throwable  |

And then one for each array type:

### Array types

| C/C++         | Java                 |
| ------------- | -------------------- |
| jobjectArray  | java.lang.Object[]   |
| jbooleanArray | boolean[]            |
| jbyteArray    | byte[]               |
| jcharArray    | char[]               |
| jdoubleArray  | double[]             |
| jfloatArray   | float[]              |
| jintArray     | int[]                |
| jlongArray    | long[]               |
| jshortArray   | short[]              |

Note that although `jint` and friends just map to the corresponding C++ integral type, all the other types such as `jobject` and `jintArray` are opaque types that must be passed to JNI functions. In particular, `jintArray` **does not** correspond to a C++ `int*` and **must not** be used as such.

### The `jvalue` union

When passing arguments from C/C++ to a Java method, the `jvalue` union is used:

```c++
typedef union jvalue {
  // Primitive types.
  jboolean z;
  jbyte    b;
  jchar    c;
  jdouble  d;
  jfloat   f;
  jint     i;
  jlong    j;
  jshort   s;
  // Reference types.
  jobject  l;
} jvalue;
```

### Field/method signatures

When looking up a Java class/field/method from C/C++, you use a string.

Primitive types have single letter names, corresponding to the `jvalue` fields:

| Character | Java    |
| --------- | ------- |
| Z         | boolean |
| B         | byte    |
| C         | char    |
| D         | double  |
| F         | float   |
| I         | int     |
| J         | long    |
| S         | short   |

Classes are named using the internal format such as `java/lang/Object` rather than the more usual Java `java.lang.Object`. They must also be fully-qualified.

When a class is named as part of a signature (that is, it's the type of a field or an argument/return type for a method, rather than just naming the class for `FindClass()`), it's prefixed by `L` and suffixed by `;`.

An array is indicated by prefixing the type with `[`.

A field signature is simply one type in this format:
```java
  int i; // "I"
  float[] fs; // "[F"

  String s; // "Ljava/lang/String;"
  Object[] os; // "[Ljava/lang/Object;"
```

A method signature is a sequence of argument types surrounded by `()`, followed by a return type. The extra letter `V` is used for the `void` return type.
```java
  int f1(); // "()I"
  String f2(); // "()Ljava/lang/String;"
  float[] f3(); // "()[F"

  // "(I[FLjava/lang/String;[Ljava/lang/Object;)V"
  void f4(int i, float[] fs, String s, Object[] os);
```

### `JavaVM`

JNI defines two key data structures in `<jni.h>`: `JavaVM` and `JNIEnv`. Both of these are essentially pointers to pointers to function tables, and they're how you access JNI from C/C++.

`JavaVM` (along with a handful of global functions) implements the "Java Invocation Interface", which allow you to create and destroy a `JavaVM`. In theory you can have multiple `JavaVM`s per process, but Android only allows one. App developers are most likely to need `AttachCurrentThread()` and `DetachCurrentThread()`.

```c++
    // Used to attach/detach the calling thread.
    jint AttachCurrentThread(JavaVM*, JNIEnv**, void*);
    jint DetachCurrentThread(JavaVM*);

    // Like AttachCurrentThread(), but marks the thread as a Java "daemon" thread.
    jint AttachCurrentThreadAsDaemon(JavaVM*, JNIEnv**, void*);

    // Used to get a JNIEnv. Apps should usually call AttachCurrentThread instead.
    jint GetEnv(JavaVM*, void**, jint);

    // Used to destroy a JavaVM. Never useful for apps.
    jint DestroyJavaVM(JavaVM*);
```

### `JNIEnv`

`JNIEnv` implements the "Java Native Inferface", and contains most of what you'll use to interact with Java/Kotlin from C/C++.

This interface is _very_ large. Here's a summary (that only shows integer as representative of the primitive types):
```c++
    // Registering native methods.
    jint           RegisterNatives(JNIEnv*, jclass, const JNINativeMethod*, jint);
    jint           UnregisterNatives(JNIEnv*, jclass);

    // Exceptions.
    jint           Throw(JNIEnv*, jthrowable);
    jint           ThrowNew(JNIEnv *, jclass, const char *);
    jboolean       ExceptionCheck(JNIEnv*);
    jthrowable     ExceptionOccurred(JNIEnv*);
    void           ExceptionDescribe(JNIEnv*);
    void           ExceptionClear(JNIEnv*);
    void           FatalError(JNIEnv*, const char*);

    // Global/local/weak references.
    jobject        NewGlobalRef(JNIEnv*, jobject);
    void           DeleteGlobalRef(JNIEnv*, jobject);
    jobject        NewLocalRef(JNIEnv*, jobject);
    void           DeleteLocalRef(JNIEnv*, jobject);
    jint           EnsureLocalCapacity(JNIEnv*, jint);
    jint           PushLocalFrame(JNIEnv*, jint);
    jobject        PopLocalFrame(JNIEnv*, jobject);
    jweak          NewWeakGlobalRef(JNIEnv*, jobject);
    void           DeleteWeakGlobalRef(JNIEnv*, jweak);
    jobjectRefType GetObjectRefType(JNIEnv*, jobject);

    // Allocation.
    jobject        AllocObject(JNIEnv*, jclass);
    jobject        NewObject(JNIEnv*, jclass, jmethodID, ...);
    jobject        NewObjectV(JNIEnv*, jclass, jmethodID, va_list);
    jobject        NewObjectA(JNIEnv*, jclass, jmethodID, const jvalue*);
    jobjectArray   NewObjectArray(JNIEnv*, jsize, jclass, jobject);
    jintArray      NewIntArray(JNIEnv*, jsize);

    // Class/field/method lookup.
    jclass         FindClass(JNIEnv*, const char*);
    jfieldID       GetFieldID(JNIEnv*, jclass, const char*, const char*);
    jfieldID       GetStaticFieldID(JNIEnv*, jclass, const char*, const char*);
    jmethodID      GetMethodID(JNIEnv*, jclass, const char*, const char*);
    jmethodID      GetStaticMethodID(JNIEnv*, jclass, const char*, const char*);
    // Field/method IDs interoperability with Java reflection.
    jmethodID      FromReflectedMethod(JNIEnv*, jobject);
    jfieldID       FromReflectedField(JNIEnv*, jobject);
    jobject        ToReflectedMethod(JNIEnv*, jclass, jmethodID, jboolean);
    jobject        ToReflectedField(JNIEnv*, jclass, jfieldID, jboolean);

    // Method invocation.
    // (There are also Nonvirtual and Static variants of these.)
    jobject        CallObjectMethod(JNIEnv*, jobject, jmethodID, ...);
    jobject        CallObjectMethodV(JNIEnv*, jobject, jmethodID, va_list);
    jobject        CallObjectMethodA(JNIEnv*, jobject, jmethodID, const jvalue*);
    jint           CallIntMethod(JNIEnv*, jobject, jmethodID, ...);
    jint           CallIntMethodV(JNIEnv*, jobject, jmethodID, va_list);
    jint           CallIntMethodA(JNIEnv*, jobject, jmethodID, const jvalue*);
    void           CallVoidMethod(JNIEnv*, jobject, jmethodID, ...);
    void           CallVoidMethodV(JNIEnv*, jobject, jmethodID, va_list);
    void           CallVoidMethodA(JNIEnv*, jobject, jmethodID, const jvalue*);

    // Field access.
    // (There are also Static variants of these.)
    jobject        GetObjectField(JNIEnv*, jobject, jfieldID);
    jint           GetIntField(JNIEnv*, jobject, jfieldID);
    void           SetObjectField(JNIEnv*, jobject, jfieldID, jobject);
    void           SetIntField(JNIEnv*, jobject, jfieldID, jint);

    // Strings.
    jstring        NewString(JNIEnv*, const jchar*, jsize);
    jsize          GetStringLength(JNIEnv*, jstring);
    const jchar*   GetStringChars(JNIEnv*, jstring, jboolean*);
    void           ReleaseStringChars(JNIEnv*, jstring, const jchar*);
    jstring        NewStringUTF(JNIEnv*, const char*);
    jsize          GetStringUTFLength(JNIEnv*, jstring);
    const char*    GetStringUTFChars(JNIEnv*, jstring, jboolean*);
    void           ReleaseStringUTFChars(JNIEnv*, jstring, const char*);
    void           GetStringRegion(JNIEnv*, jstring, jsize, jsize, jchar*);
    void           GetStringUTFRegion(JNIEnv*, jstring, jsize, jsize, char*);
    const jchar*   GetStringCritical(JNIEnv*, jstring, jboolean*);
    void           ReleaseStringCritical(JNIEnv*, jstring, const jchar*);

    // Arrays.
    jsize          GetArrayLength(JNIEnv*, jarray);
    void*          GetPrimitiveArrayCritical(JNIEnv*, jarray, jboolean*);
    void           ReleasePrimitiveArrayCritical(JNIEnv*, jarray, void*, jint);
    jobject        GetObjectArrayElement(JNIEnv*, jobjectArray, jsize);
    void           SetObjectArrayElement(JNIEnv*, jobjectArray, jsize, jobject);
    jint*          GetIntArrayElements(JNIEnv*, jintArray, jboolean*);
    void           ReleaseIntArrayElements(JNIEnv*, jintArray, jint*, jint);
    void           GetIntArrayRegion(JNIEnv*, jintArray, jsize, jsize, jint*);
    void           SetIntArrayRegion(JNIEnv*, jintArray, jsize, jsize, const jint*);

    // Misc.
    jclass         DefineClass(JNIEnv*, const char*, jobject, const jbyte*, jsize);
    jint           GetVersion(JNIEnv*);
    jint           GetJavaVM(JNIEnv*, JavaVM**);
    jclass         GetSuperclass(JNIEnv*, jclass);
    jboolean       IsAssignableFrom(JNIEnv*, jclass, jclass);
    jclass         GetObjectClass(JNIEnv*, jobject);
    jboolean       IsInstanceOf(JNIEnv*, jobject, jclass);
    jboolean       IsSameObject(JNIEnv*, jobject, jobject);

    // Synchronization.
    jint           MonitorEnter(JNIEnv*, jobject);
    jint           MonitorExit(JNIEnv*, jobject);

    // NIO DirectByteBuffer support.
    jobject        NewDirectByteBuffer(JNIEnv*, void*, jlong);
    void*          GetDirectBufferAddress(JNIEnv*, jobject);
    jlong          GetDirectBufferCapacity(JNIEnv*, jobject);
```

### Threads and `JNIEnv`

Threads can be created from managed code by `Thread.start()`, but they can also be created by [`pthread_create(3)`](http://man7.org/linux/man-pages/man3/pthread_create.3.html) or [`std::thread`](https://en.cppreference.com/w/cpp/thread/thread). A thread created from managed code is already known to the Java runtime, but other threads must be registered with it. This is known as "attaching" the thread. A thread can be attached to a `JavaVM` using the `AttachCurrentThread()` or `AttachCurrentThreadAsDaemon()` functions ("daemon" in this context refers to the `java.lang.Thread` notion of a daemon thread).

Until a thread is attached, it has no `JNIEnv` and **cannot make JNI calls**.

The other way to get hold of a `JNIEnv` is that a native method receives a `JNIEnv*` as its first argument when it's invoked. You can use that until you return from that native method, but only on that thread:

* **You cannot share a `JNIEnv` between threads.** They're implemented using thread-local storage, and you must use the correct `JNIEnv` for the thread you're on. If you communicate with a worker thread, say, that thread will need to be independently attached and use its own `JNIEnv`.
* **You cannot store a `JNIEnv` and use it later.** A `JNIEnv` is only valid for the duration of your native function.

By contrast, it is allowed to share a `JavaVM`. A function can access its `JNIEnv` using `java_vm->GetEnv()` , but that's only valid if the calling thread is actually attached. It's usually easiest to just call `AttachCurrentThread()`.

Calling `AttachCurrentThread()` on an already-attached thread is a cheap no-op (basically just a read of a thread-local variable).

Android does not suspend threads executing native code. If garbage collection is in progress, or the debugger has issued a suspend request, Android will pause the thread the next time it makes a JNI call.

Threads attached through JNI **must call `DetachCurrentThread()` before they exit** to avoid leaking resources. To prevent this kind of bug, Android will abort your process if a thread exits while still attached. If coding this directly is awkward, you can use [`pthread_key_create(3)`](http://man7.org/linux/man-pages/man3/pthread_key_create.3p.html) to define a destructor function that will be called before the thread exits, and call `DetachCurrentThread()` from there. Use that key with `pthread_setspecific()` to store the JNIEnv in thread-local-storage; that way it'll be passed into your destructor as the argument.


#### Trade-off: creating threads from managed code or native code?

It's usually best to use `Thread.start()` to create any thread that needs to call in to Java code. Doing so will ensure:

1. That you have sufficient stack space to call into managed code.
2. That you're in the correct `ThreadGroup`. Attaching a natively-created thread causes a corresponding `java.lang.Thread` object to be constructed and added to the "main" `ThreadGroup`.
3. That you're using the same `ClassLoader` as your Java code. This is important if you cause classes to be loaded (either by calling `FindClass()` or as a side-effect of calling into managed code). If you're in a position where you really need to use threads created by managed code but are having difficulties with class loaders, see `JNI_OnLoad` for a likely solution.

Your native threads will be visible to the debugger once they're attached, but it's easier to set a thread's name from managed code than from native code. See [`pthread_setname_np(3)`](http://man7.org/linux/man-pages/man3/pthread_setname_np.3.html) if you have a `pthread_t` or `thread_t`, or use [`std::thread::native_handle()`](https://en.cppreference.com/w/cpp/thread/thread/native_handle) to turn a `std::thread` into a `pthread_t`.


### C/C++ `JNIEnv` syntax differences

The C declaration of `JNIEnv` (and `JavaVM`) is different from the C++ declaration. In C they're just a pointer to a table of function pointers. In C++, they're wrapped with classes that have a member function for each JNI function that indirects through the table, giving more natural syntax.

This document uses C++ rather than C, so a call to `NewStringUTF()` looks like this:
```c+++
  jstring s = env->NewStringUTF("hello");
```
In C that would have to be this instead (with `env` mentioned twice):
```c
  jstring s = (*env)->NewStringUTF(env, "hello");
```

If you have a header file that refers to `JNIEnv` and is used by both C and C++, you will have to do some extra work to paper over the different types used in C and C++. For app rather than library code, it's usually easier to only support one of C or C++ in your headers.




## Finding native methods

When a Java method declared `native` is invoked, the runtime needs to find the corresponding C/C++ function pointer. There are two ways in which this can happen: either by convention (by using the right name), or by configuration (with `RegisterNatives()`).

### Finding native methods by naming convention

ART can look up your native method implementation at run time using [dlsym(3)](http://man7.org/linux/man-pages/man3/dlsym.3.html). For method `m` of class `C` in package `a.b`, the name would be `Java_a_b_C_m`. There are special rules for non-ASCII characters or for names that themselves contain `_` or for overloaded methods (which aren't representable in C), but you can use `javac -h` to generate a header file for any native methods if in doubt. (TODO: doesn't Android Studio have some built in support for this too?)

### Finding native methods via `RegisterNatives`

A better alternative is to use `RegisterNatives` to explicitly match.




--------------------





## TODO

Find a home for/expand on these:

* `JNI_OnLoad`

  * Copy existing `JNI_OnLoad` example from https://developer.android.com/training/articles/perf-jni#native-libraries 

  * Explain that `JNI_VERSION_1_6` is latest, and that it’s like targetApiLevel for compat.

* Always use `extern "C"` with `JNIEXPORT`.

* Recommend one big library (http://b/133823482).

* To browse global JNI references and see where global JNI references are created and deleted, use
the "JNI heap" view in the Android Studio [Memory Profiler](https://developer.android.com/studio/profile/memory-profiler#jni).

* `JNI_TRUE`/`JNI_FALSE`

* `JNI_OK`/`JNI_ERR` and friends.

* Incorporate the go/jni-scoped-ref docs.



## General tips

Try to minimize the footprint of your JNI layer. There are several dimensions to consider here. Your JNI solution should try to follow these guidelines (listed below by order of importance, beginning with the most important):

  * **Minimize marshalling of resources across the JNI layer.** Marshalling across the JNI layer has non-trivial costs. Try to design an interface that minimizes the amount of data you need to marshall and the frequency with which you must marshall data.
  * **Avoid asynchronous communication between code written in a managed programming language and code written in C++ when possible.** This will keep your JNI interface easier to maintain. You can typically simplify asynchronous UI updates by keeping the async update in the same language as the UI. For example, instead of invoking a C++ function from the UI thread in the Java code via JNI, it's better to do a callback between two threads in the Java programming language, with one of them making a blocking C++ call and then notifying the UI thread when the blocking call is complete.
  * **Minimize the number of threads that need to touch or be touched by JNI.** If you do need to utilize thread pools in both the Java and C++ languages, try to keep JNI communication between the pool owners rather than between individual worker threads.
  * **Keep your interface code in a low number of easily identified C++ and Java source locations to facilitate future refactors.** Consider using a JNI auto-generation library as appropriate.




## Using `jclass`, `jmethodID`, and `jfieldID`

If you want to access an object's field from native code, you would do the following:

* Get the class object reference for the class with `FindClass()`.
* Get the field ID for the field with `GetFieldID()` or `GetStaticFieldID()`.
* Get the contents of the field with something appropriate, such as `GetIntField()`.

Similarly, to call a method, you'd first get a class object reference and then a method ID.  The IDs are often just pointers to internal runtime data structures.  Looking them up may require several string comparisons, but once you have them the actual call to get the field or invoke the method is very quick.

If performance is important, it's useful to look the values up once and cache the results in your native code.  Because there is a limit of one JavaVM per process, it's reasonable to store this data in a static local structure.

The class references, field IDs, and method IDs are guaranteed valid until the class is unloaded.  Classes are only unloaded if all classes associated with a ClassLoader can be garbage collected, which is rare but will not be impossible in Android.  Note however that the `jclass` is a class reference and **must be protected** with a call to `NewGlobalRef` (see the next section).

If you would like to cache the IDs when a class is loaded, and automatically re-cache them if the class is ever unloaded and reloaded, the correct way to initialize the IDs is to add a piece of code that looks like this to the appropriate class:

```java
    /*
     * We use a class initializer to allow the native code to cache some
     * field offsets. This native function looks up and caches interesting
     * class/field/method IDs. Throws on failure.
     */
    private static native void nativeInit();

    static {
        nativeInit();
    }
```

Create a `nativeClassInit` method in your C/C++ code that performs the ID lookups.  The code will be executed once, when the class is initialized.  If the class is ever unloaded and then reloaded, it will be executed again.

## Local and global references

Every argument passed to a native method, and almost every object returned by a JNI function is a "local reference".  This means that it's valid for the duration of the current native method in the current thread. **Even if the object itself continues to live on after the native method returns, the reference is not valid.**

This applies to all sub-classes of `jobject`, including `jclass`, `jstring`, and `jarray`. (The runtime will warn you about most reference mis-uses when extended JNI checks are enabled.)

The only way to get non-local references is via the functions `NewGlobalRef` and `NewWeakGlobalRef`.

If you want to hold on to a reference for a longer period, you must use a "global" reference. The `NewGlobalRef` function takes the local reference as an argument and returns a global one. The global reference is guaranteed to be valid until you call `DeleteGlobalRef`.

This pattern is commonly used when caching a jclass returned from `FindClass`, e.g.:

```c++
jclass localClass = env->FindClass("MyClass");
jclass globalClass = reinterpret_cast<jclass>(env->NewGlobalRef(localClass));
```

All JNI methods accept both local and global references as arguments.

It's possible for references to the same object to have different values. For example, the return values from consecutive calls to `NewGlobalRef` on the same object may be different. To see if two references refer to the same object, you must use the `IsSameObject` function. **Never compare references with `==` in native code**.

One consequence of this is that you **must not assume object references are constant or unique** in native code. The 32-bit value representing an object may be different from one invocation of a method to the next, and it's possible that two different objects could have the same 32-bit value on consecutive calls. Do not use `jobject` values as keys.

Programmers are required to "not excessively allocate" local references.  In practical terms this means that if you're creating large numbers of local references, perhaps while running through an array of objects, you should free them manually with `DeleteLocalRef` instead of letting JNI do it for you. The implementation is only required to reserve slots for 16 local references, so if you need more than that you should either delete as you go or use `EnsureLocalCapacity`/`PushLocalFrame` to reserve more.

Note that `jfieldID`s and `jmethodID`s are opaque types, not object references, and should not be passed to `NewGlobalRef`. The raw data pointers returned by functions like `GetStringUTFChars` and `GetByteArrayElements` are also not objects. (They may be passed
between threads, and are valid until the matching Release call.)

One unusual case deserves separate mention. If you attach a native thread with `AttachCurrentThread`, the code you are running will never automatically free local references until the thread detaches. Any local references you create will have to be deleted manually. In general, any native code that creates local references in a loop probably needs to do some manual deletion.

Be careful using global references. Global references can be unavoidable, but they are difficult to debug and can cause difficult-to-diagnose memory (mis)behaviors. All else being equal, a solution with fewer global references is probably better.


## UTF-8 and UTF-16 strings

The Java programming language uses UTF-16. For convenience, JNI provides methods that work with [Modified UTF-8](http://en.wikipedia.org/wiki/UTF-8#Modified_UTF-8) as well. The
modified encoding is useful for C code because it encodes \u0000 as 0xc0 0x80 instead of 0x00. The advantage of this is that you can count on having C-style zero-terminated strings, suitable for use with standard libc string functions. The disadvantage is that you cannot pass arbitrary UTF-8 data to JNI and expect it to work correctly.

If possible, it's usually faster to operate with UTF-16 strings. Android currently does not require a copy in `GetStringChars`, whereas `GetStringUTFChars` requires an allocation and a conversion to UTF-8. Note that **UTF-16 strings are not zero-terminated**, and `\u0000` is allowed, so you need to hang on to the string length as well as the `jchar*`.

**Don't forget to call the release function on the strings you get.** The string functions return `jchar*` or `jbyte*`, which are C-style pointers to primitive data rather than local references. They are guaranteed valid until release is called, which means they are not released when the native method returns.

**Data passed to `NewStringUTF()` must be in Modified UTF-8 format.** A common mistake is reading character data from a file or network stream and handing it to `NewStringUTF()` without filtering it. Unless you know the data is valid MUTF-8 (or 7-bit ASCII, which is a compatible subset), you need to strip out invalid characters or convert them to proper Modified UTF-8 form. If you don't, the UTF-16 conversion is likely to provide unexpected results. CheckJNI -- which is on by default for emulators -- scans strings and aborts the VM if it receives invalid input.

## Primitive arrays

JNI provides functions for accessing the contents of array objects. While arrays of objects must be accessed one entry at a time, arrays of primitives can be read and written directly as if they were declared in C.

To make the interface as efficient as possible without constraining the VM implementation, the `Get<PrimitiveType>ArrayElements()` family of calls allows the runtime to either return a pointer to the actual elements, or allocate some memory and make a copy. Either way, the raw pointer returned is guaranteed to be valid until the corresponding `Release<PrimitiveType>ArrayElements()` call is issued (which implies that, if the data wasn't copied, the array object will be pinned down and can't be relocated as part of compacting the heap).

**You must release every array you get.**  Also, if the get call fails, you must ensure that your code doesn't try to release a null pointer later.

You can determine whether or not the data was copied by passing in a non-NULL pointer for the boolean `isCopy` argument.  This is rarely useful.

The release call takes a mode argument that can have one of three values. The actions performed by the runtime depend upon whether it returned a pointer to the actual data or a copy of it:

* `0`
    * Actual: the array object is un-pinned.
    * Copy: data is copied back.  The buffer with the copy is freed.
* `JNI_COMMIT`
    * Actual: does nothing.
    * Copy: data is copied back.  The buffer with the copy **is not freed**.
* `JNI_ABORT`
    * Actual: the array object is un-pinned. Earlier writes are **not** aborted.
    * Copy: the buffer with the copy is freed; any changes to it are lost.

One reason for checking the `isCopy` flag is to know if you need to call release with `JNI_COMMIT` after making changes to an array --- if you're alternating between making changes and executing code that uses the contents of the array, you may be able to skip the no-op commit. Another possible reason for checking the flag is for efficient handling of `JNI_ABORT`. For example, you might want to get an array, modify it in place, pass pieces to other functions, and then discard the changes. If you know that JNI is making a new copy for you, there's no need to create another "editable" copy. If JNI is passing
you the original, then you do need to make your own copy.

It is a common mistake (often repeated in example code) to assume that you can skip the release call if `*isCopy` is false.  This is not the case.  If no copy buffer was allocated, then the original memory must be pinned down and can't be moved by the garbage collector.

Also note that the `JNI_COMMIT` flag does **not** release the array, and you will need to call release again with a different flag eventually.


## Array region calls

There is an alternative to calls like `GetXArrayElements` and `GetStringChars` that may be very helpful when all you want to do is copy data in or out.  Consider the following:

```c++
    jbyte* data = env->GetByteArrayElements(array, nullptr);
    if (data != nullptr) {
        memcpy(buffer, data, n);
        env->ReleaseByteArrayElements(array, data, JNI_ABORT);
    }
```

This grabs the array, copies the first `n` bytes out of it, and then releases the array.  Depending upon the implementation, the "get" call will either pin or copy the array
contents. The code copies the data with `memcpy(3)` (for perhaps a second time), then makes the "release" call >; in this case `JNI_ABORT` ensures there's no chance of a third copy.

One can accomplish the same thing more directly:
```c++
    env->GetByteArrayRegion(array, 0, n, buffer);
```

This has several advantages:

* Requires one JNI call instead of 2, reducing overhead.
* Doesn't require pinning or extra data copies.
* Reduces the risk of programmer error &mdash; no risk of forgetting to call release after something fails.

Similarly, you can use the `SetXArrayRegion` call to copy data into an array, and `GetStringRegion` or `GetStringUTFRegion` to copy characters out of a `String`.


<a name="exceptions" id="exceptions"></a>
<h2>Exceptions</h2>

<p><strong>You must not call most JNI functions while an exception is pending.</strong>
Your code is expected to notice the exception (via the function's return value,
<code>ExceptionCheck</code>, or <code>ExceptionOccurred</code>) and return,
or clear the exception and handle it.</p>

<p>The only JNI functions that you are allowed to call while an exception is
pending are:</p>
<ul>
    <li><code>DeleteGlobalRef</code>
    <li><code>DeleteLocalRef</code>
    <li><code>DeleteWeakGlobalRef</code>
    <li><code>ExceptionCheck</code>
    <li><code>ExceptionClear</code>
    <li><code>ExceptionDescribe</code>
    <li><code>ExceptionOccurred</code>
    <li><code>MonitorExit</code>
    <li><code>PopLocalFrame</code>
    <li><code>PushLocalFrame</code>
    <li><code>ReleaseXArrayElements</code>
    <li><code>ReleasePrimitiveArrayCritical</code>
    <li><code>ReleaseStringChars</code>
    <li><code>ReleaseStringCritical</code>
    <li><code>ReleaseStringUTFChars</code>
</ul>

<p>Many JNI calls can throw an exception, but often provide a simpler way
of checking for failure.  For example, if <code>NewString</code> returns
a non-NULL value, you don't need to check for an exception.  However, if
you call a method (using a function like <code>CallObjectMethod</code>),
you must always check for an exception, because the return value is not
going to be valid if an exception was thrown.</p>

<p>Note that exceptions thrown by interpreted code do not unwind native stack
frames, and Android does not yet support C++ exceptions.
The JNI <code>Throw</code> and <code>ThrowNew</code> instructions just
set an exception pointer in the current thread.  Upon returning to managed
from native code, the exception will be noted and handled appropriately.</p>

<p>Native code can "catch" an exception by calling <code>ExceptionCheck</code> or
<code>ExceptionOccurred</code>, and clear it with
<code>ExceptionClear</code>.  As usual,
discarding exceptions without handling them can lead to problems.</p>

<p>There are no built-in functions for manipulating the <code>Throwable</code> object
itself, so if you want to (say) get the exception string you will need to
find the <code>Throwable</code> class, look up the method ID for
<code>getMessage "()Ljava/lang/String;"</code>, invoke it, and if the result
is non-NULL use <code>GetStringUTFChars</code> to get something you can
hand to <code>printf(3)</code> or equivalent.</p>


<a name="extended_checking" id="extended_checking"></a>
<h2>Extended checking</h2>

<p>JNI does very little error checking. Errors usually result in a crash. Android also offers a mode called CheckJNI, where the JavaVM and JNIEnv function table pointers are switched to tables of functions that perform an extended series of checks before calling the standard implementation.</p>

<p>The additional checks include:</p>

<ul>
<li>Arrays: attempting to allocate a negative-sized array.</li>
<li>Bad pointers: passing a bad jarray/jclass/jobject/jstring to a JNI call, or passing a NULL pointer to a JNI call with a non-nullable argument.</li>
<li>Class names: passing anything but the “java/lang/String” style of class name to a JNI call.</li>
<li>Critical calls: making a JNI call between a “critical” get and its corresponding release.</li>
<li>Direct ByteBuffers: passing bad arguments to <code>NewDirectByteBuffer</code>.</li>
<li>Exceptions: making a JNI call while there’s an exception pending.</li>
<li>JNIEnv*s: using a JNIEnv* from the wrong thread.</li>
<li>jfieldIDs: using a NULL jfieldID, or using a jfieldID to set a field to a value of the wrong type (trying to assign a StringBuilder to a String field, say), or using a jfieldID for a static field to set an instance field or vice versa, or using a jfieldID from one class with instances of another class.</li>
<li>jmethodIDs: using the wrong kind of jmethodID when making a <code>Call*Method</code> JNI call: incorrect return type, static/non-static mismatch, wrong type for ‘this’ (for non-static calls) or wrong class (for static calls).</li>
<li>References: using <code>DeleteGlobalRef</code>/<code>DeleteLocalRef</code> on the wrong kind of reference.</li>
<li>Release modes: passing a bad release mode to a release call (something other than <code>0</code>, <code>JNI_ABORT</code>, or <code>JNI_COMMIT</code>).</li>
<li>Type safety: returning an incompatible type from your native method (returning a StringBuilder from a method declared to return a String, say).</li>
<li>UTF-8: passing an invalid <a href="http://en.wikipedia.org/wiki/UTF-8#Modified_UTF-8">Modified UTF-8</a> byte sequence to a JNI call.</li>
</ul>

<p>(Accessibility of methods and fields is still not checked: access restrictions don't apply to native code.)</p>

<p>There are several ways to enable CheckJNI.</p>

<p>If you’re using the emulator, CheckJNI is on by default.</p>

<p>If you have a rooted device, you can use the following sequence of commands to restart the runtime with CheckJNI enabled:</p>

<pre class="prettyprint lang-bash">adb shell stop
adb shell setprop dalvik.vm.checkjni true
adb shell start</pre>

<p>In either of these cases, you’ll see something like this in your logcat output when the runtime starts:</p>

<pre class="none">D AndroidRuntime: CheckJNI is ON</pre>

<p>If you have a regular device, you can use the following command:</p>

<pre class="prettyprint lang-bash">adb shell setprop debug.checkjni 1</pre>

<p>This won’t affect already-running apps, but any app launched from that point on will have CheckJNI enabled. (Change the property to any other value or simply rebooting will disable CheckJNI again.) In this case, you’ll see something like this in your logcat output the next time an app starts:</p>

<pre class="none">D Late-enabling CheckJNI</pre>

<p>You can also set the <code>android:debuggable</code> attribute in your application's manifest to
turn on CheckJNI just for your app. Note that the Android build tools will do this automatically for
certain build types.


## Native libraries

You can load native code from shared libraries with the standard
<a href="/reference/java/lang/System.html#loadLibrary(java.lang.String)"><code>System.loadLibrary</code></a>.

In practice, older versions of Android had bugs in PackageManager that caused installation and update of native libraries to be unreliable. The [ReLinker](https://github.com/KeepSafe/ReLinker) project offers workarounds for this and other native library loading problems.

Call `System.loadLibrary` (or `ReLinker.loadLibrary`) from a static class initializer. The argument is the "undecorated" library name, so to load `libfoo.so` you would pass in `"foo"`.

If you have only one class with native methods, it makes sense for the call to `System.loadLibrary` to be in a static initializer for that class. Otherwise you might want to make the call from `Application` so you know that the library is always loaded, and always loaded early.

There are two ways that the runtime can find your native methods. You can either explicitly register them with `RegisterNatives`, or you can let the runtime look them up dynamically with `dlsym`. The advantages of `RegisterNatives` are that you get up-front checking that the symbols exist, plus you can have smaller and faster shared libraries by not exporting anything but `JNI_OnLoad`. The advantage of letting the runtime discover your functions is that it's slightly less code to write.

To use `RegisterNatives`:

* Provide a `JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved)` function.
* In your `JNI_OnLoad`, register all of your native methods using `RegisterNatives`.
* Build with `-fvisibility=hidden` so that only your `JNI_OnLoad` is exported from your library. This produces faster and smaller code, and avoids potential collisions with other libraries loaded into your app (but it creates less useful stack traces if you app crashes in native code).

The static initializer should look like this:

```java
static {
    System.loadLibrary("foo");
}
```

The `JNI_OnLoad` function should look something like this if written in C++:
```c++
extern "C" JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
  JNIEnv* env;
  if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
    return JNI_ERR;
  }

  // Find your class.
  // JNI_OnLoad is called from the correct class loader context for this to work.
  jclass c = env->FindClass("com/example/app/package/MyClass");
  if (c == nullptr) return JNI_ERR;

  // Register your class' native methods.
  static const JNINativeMethod methods[] = {
    {"nativeFoo", "()V", reinterpret_cast<void*>(nativeFoo)},
    {"nativeBar", "(Ljava/lang/String;I)Z", reinterpret_cast<void*>(nativeBar)},
  };
  int rc = env->RegisterNatives(c, methods, sizeof(methods)/sizeof(JNINativeMethod));
  if (rc != JNI_OK) return rc;

  return JNI_VERSION_1_6;
}
```

To instead use "discovery" of native methods, you need to name them in a specific way (see <a href="http://java.sun.com/javase/6/docs/technotes/guides/jni/spec/design.html#wp615">the JNI spec</a> for details). This means that if a method signature is wrong, you won't know about it until the first time the method is actually invoked.

Any `FindClass()` calls made from `JNI_OnLoad` will resolve classes in the context of the class loader that was used to load the shared library. When called from other contexts, `FindClass` uses the class loader associated with the method at the top of the Java stack, or if there isn't one (because the call is from a native thread that was just attached) it uses the "system" class loader. The system class loader does not know about your application's classes, so you won't be able to look up your own classes with `FindClass` in that context. This makes `JNI_OnLoad` a convenient place to look up and cache classes: once you have a valid `jclass` you can use it from any attached thread.


## 64-bit considerations

To support architectures that use 64-bit pointers, use a `long` field rather than an `int` when storing a pointer to a native structure in a Java field.


## Unsupported features/backwards compatibility

All JNI 1.6 features are supported, with the following exception:
<ul>
    <li><code>DefineClass</code> is not implemented.  Android does not use
    Java bytecodes or class files, so passing in binary class data
    doesn't work.</li>
</ul>

<p>For backward compatibility with older Android releases, you may need to
be aware of:</p>
<ul>
    <p>In Android versions prior to <a href="/about/versions/oreo/index.html">Android 8.0 (Oreo)</a>, the
    number of local references is capped at a version-specific limit. Beginning in Android 8.0,
    Android supports unlimited local references.</p></li>
</ul>


<a name="faq_ULE" id="faq_ULE"></a>
<h2>FAQ: Why do I get <code>UnsatisfiedLinkError</code>?</h2>

<p>When working on native code it's not uncommon to see a failure like this:</p>
<pre class="none">java.lang.UnsatisfiedLinkError: Library foo not found</pre>

<p>In some cases it means what it says &mdash; the library wasn't found.  In
other cases the library exists but couldn't be opened by <code>dlopen(3)</code>, and
the details of the failure can be found in the exception's detail message.</p>

Common reasons why you might encounter "library not found" exceptions:

* The library doesn't exist or isn't accessible to the app. Use `adb shell ls -l path` to check its presence and permissions.
* The library wasn't built with the NDK. This can result in dependencies on functions or libraries that don't exist on the device.

Another class of `UnsatisfiedLinkError` failures looks like:
```
java.lang.UnsatisfiedLinkError: myfunc
        at Foo.myfunc(Native Method)
        at Foo.main(Foo.java:10)</pre>
```

In logcat, you'll see:
```
W/dalvikvm(  880): No implementation found for native LFoo;.myfunc ()V
```

This means that the runtime tried to find a matching method but was unsuccessful. Some common reasons for this are:

* The library isn't getting loaded. Check the logcat output for messages about library loading.
* The method isn't being found due to a name or signature mismatch. This is commonly caused by:
    * For lazy method lookup, failing to declare C++ functions with `extern "C"` and appropriate visibility (`JNIEXPORT`). You can use `nm(1)` to see the symbols as they appear in the library; if they look mangled (something like `_Z15Java_Foo_myfuncP7_JNIEnvP7_jclass` rather than `Java_Foo_myfunc`), or if the symbol type is a lowercase 't' rather than an uppercase 'T', then you need to      adjust the declaration.
    * For explicit registration, minor errors when entering the method signature. Make sure that what you're passing to the registration call matches the signature in the log file. Remember that `B` is `byte` and `Z` is `boolean`. Class name components in signatures start with `L`, end with `;`, use '/' to separate package/class names, and use `$` to separate inner-class names (`Ljava/util/Map$Entry;`, say).

Using `javah` (or `javac -h`) to automatically generate JNI headers may help avoid some problems. Android Studio can also automate this.


<a name="faq_FindClass" id="faq_FindClass"></a>
<h2>FAQ: Why didn't <code>FindClass</code> find my class?</h2>

<p>(Most of this advice applies equally well to failures to find methods
with <code>GetMethodID</code> or <code>GetStaticMethodID</code>, or fields
with <code>GetFieldID</code> or <code>GetStaticFieldID</code>.)</p>

<p>Make sure that the class name string has the correct format.  JNI class
names start with the package name and are separated with slashes,
such as <code>java/lang/String</code>.  If you're looking up an array class,
you need to start with the appropriate number of square brackets and
must also wrap the class with 'L' and ';', so a one-dimensional array of
<code>String</code> would be <code>[Ljava/lang/String;</code>.
If you're looking up an inner class, use '$' rather than '.'. In general,
using <code>javap</code> on the .class file is a good way to find out the
internal name of your class.</p>

If you enable code shrinking, make sure that you
  <a href="/studio/build/shrink-code.html#keep-code">configure which code to keep</a>. Configuring proper keep rules is important because the code shrinker might otherwise remove classes, methods, or fields that are only used from JNI.

If the class name looks right, you could be running into a class loader issue.  `FindClass`wants to start the class search in the class loader associated with your code.  It examines the call stack, which will look something like:

```
    Foo.myfunc(Native Method)
    Foo.main(Foo.java:10)
```

<p>The topmost method is <code>Foo.myfunc</code>.  <code>FindClass</code>
finds the <code>ClassLoader</code> object associated with the <code>Foo</code>
class and uses that.</p>

<p>This usually does what you want.  You can get into trouble if you
create a thread yourself (perhaps by calling <code>pthread_create</code>
and then attaching it with <code>AttachCurrentThread</code>). Now there
are no stack frames from your application.
If you call <code>FindClass</code> from this thread, the
JavaVM will start in the "system" class loader instead of the one associated
with your application, so attempts to find app-specific classes will fail.</p>

<p>There are a few ways to work around this:</p>
<ul>
    <li>Do your <code>FindClass</code> lookups once, in
    <code>JNI_OnLoad</code>, and cache the class references for later
    use.  Any <code>FindClass</code> calls made as part of executing
    <code>JNI_OnLoad</code> will use the class loader associated with
    the function that called <code>System.loadLibrary</code> (this is a
    special rule, provided to make library initialization more convenient).
    If your app code is loading the library, <code>FindClass</code>
    will use the correct class loader.
    <li>Pass an instance of the class into the functions that need
    it, by declaring your native method to take a Class argument and
    then passing <code>Foo.class</code> in.
    <li>Cache a reference to the <code>ClassLoader</code> object somewhere
    handy, and issue <code>loadClass</code> calls directly.  This requires
    some effort.
</ul>


## FAQ: How do I share raw data with native code?

You may find yourself in a situation where you need to access a large buffer of raw data from both managed and native code. Common examples include manipulation of bitmaps or sound samples. There are two basic approaches.

You can store the data in a `byte[]`. This allows very fast access from managed code.  On the native side, however, you're not guaranteed to be able to access the data without having to copy it. In some implementations, `GetByteArrayElements` and `GetPrimitiveArrayCritical` will return actual pointers to the raw data in the managed heap, but in others it will allocate a buffer on the native heap and copy the data over.

The alternative is to store the data in a direct byte buffer. These can be created with `java.nio.ByteBuffer.allocateDirect`, or the JNI `NewDirectByteBuffer` function. Unlike regular byte buffers, the storage is not allocated on the managed heap, and can always be accessed directly from native code (get the address with `GetDirectBufferAddress`). Depending on how direct byte buffer access is implemented, accessing the data from managed code can be very slow.

The choice of which to use depends on two factors:

1. Will most of the data accesses happen from code written in Java or in C/C++?
2. If the data is eventually being passed to a system API, what form must it be in? (For example, if the data is eventually passed to a function that takes a byte[], doing processing in a direct `ByteBuffer` might be unwise.)

If there's no clear winner, use a direct byte buffer. Support for them is built directly into JNI, and performance should improve in future releases.