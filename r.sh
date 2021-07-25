~/bin/adb root;
~/bin/adb remount;
~/bin/adb push /home/uqtony/data2_project/rk3288_android_71/ismart7.1/out/target/product/rk3288/system/xbin/rgb_led_matrix /sdcard/;
~/bin/adb shell cp /sdcard/rgb_led_matrix /system/xbin/;
~/bin/adb shell chmod 777 /system/xbin/rgb_led_matrix;
~/bin/adb shell "echo 160 > /sys/class/gpio/export";
~/bin/adb shell /system/xbin/rgb_led_matrix;
~/bin/adb shell cat /sys/class/gpio/gpio160/value;
