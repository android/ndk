#include <jni.h>

int bar();

jint foo(JNIEnv*  env, jobject  this)
{
	return bar();
	return 0;
}
