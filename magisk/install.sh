#!/sbin/sh

SKIPMOUNT=false
PROPFILE=false
POSTFSDATA=false
LATESTARTSERVICE=true
REPLACE=

print_modname() {
  ui_print "*******************************"
  ui_print "           Foxy Boot           "
  ui_print "*******************************"
}

on_install() {
  ui_print "- Checking Android version"
  if [ $API -ge 26 ]; then
    abort "! Boot animation cannot be replaced with Magisk on Android 8.0 and higher!"
  fi

  ui_print "- Checking libc++ presence"

  [ -n "`find /system/lib* -name libc++.so`" ] ||
  abort "! Unable to find libc++.so!"

  local ABI=
  case $ARCH in
    arm) ABI=armeabi-v7a ;;
    arm64) ABI=arm64-v8a ;;
    x86) ABI=x86 ;;
    x64) ABI=x86_64 ;;
  esac

  ui_print "- Extracting module files"

  mkdir -p $MODPATH/system/bin
  unzip -o "$ZIPFILE" bootanimation-$ABI -d $MODPATH/system/bin >&2
  mv $MODPATH/system/bin/bootanimation-$ABI $MODPATH/system/bin/bootanimation >&2
  [ -e $MODPATH/system/bin/bootanimation ] ||
  abort "! Unable to extract bootanimation!"

  ui_print "- Checking the binary"

  chmod 755 $MODPATH/system/bin/bootanimation
  local OUT
  OUT="`$MODPATH/system/bin/bootanimation ldcheck 2>&1`"
  if [ $? -ne 0 ]; then
    for sym in `echo "$OUT" | grep -o '"[^"]*"' | grep -v '/'`; do
      ui_print "cannot locate $sym"
    done
    abort '! Unable to link the executable!'
  fi
}

set_permissions() {
  set_perm_recursive $MODPATH 0 0 0755 0644
  set_perm $MODPATH/system/bin/bootanimation 0 2000 0755 u:object_r:bootanim_exec:s0
}
