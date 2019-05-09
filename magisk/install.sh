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
  mv $MODPATH/system/bin/bootanimation-$ABI $MODPATH/system/bin/foxy-boot >&2
  [ -e $MODPATH/system/bin/foxy-boot ] ||
  abort "! Unable to extract bootanimation!"

  # create symlink to bypass bootanimation permissions reset
  ln -sf foxy-boot $MODPATH/system/bin/bootanimation

  ui_print "- Checking the binary"

  chmod 755 $MODPATH/system/bin/foxy-boot
  local OUT
  OUT="`$MODPATH/system/bin/foxy-boot ldcheck 2>&1`"
  if [ $? -ne 0 ]; then
    for sym in `echo "$OUT" | grep -o '"[^"]*"' | grep -v '/'`; do
      ui_print "cannot locate $sym"
    done
    abort '! Unable to link the executable!'
  fi
}

set_permissions() {
  set_perm_recursive $MODPATH 0 0 0755 0644
  set_perm $MODPATH/system/bin/foxy-boot 0 1000 2755 u:object_r:bootanim_exec:s0
}
