## after upgrade chromium source, must copy chromium/src/out/arm64/snapshot_blob.bin  to app/src/main/assets/snapshot_blob_64.bin
all:
	JAVA_HOME=/home/user/dev/jdk-11.0.13/ ./gradlew assemble
clean:
	JAVA_HOME=/home/user/dev/jdk-11.0.13/ ./gradlew clean
install:
	adb install -r ./app/build/outputs/apk/debug/app-debug.apk
