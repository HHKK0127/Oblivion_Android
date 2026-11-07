# JNI entry points and callback classes are resolved by class/method name.
-keep class com.example.oblivion.GameRenderer { *; }
-keep class com.example.oblivion.VideoBridge { *; }

# Native audio callbacks look up these instance methods through JNI. Keep the
# component class name as well as the method names for FindClass/GetMethodID.
-keep class com.example.oblivion.MainActivity {
    public void playBGM(java.lang.String);
    public void playSE(java.lang.String);
    public void stopBGM();
}
