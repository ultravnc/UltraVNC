# Keep all librats public API
-keep public class com.librats.** { *; }

# Keep callback interfaces for consumer apps
-keep interface com.librats.*Callback { *; }
