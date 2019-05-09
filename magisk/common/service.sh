#!/system/bin/sh
while true; do
  magiskpolicy --live \
  'allow bootanim kernel system syslog_read' \
  'allow bootanim logdr_socket sock_file write' \
  'allow bootanim logd unix_stream_socket connectto' \
  2>&1 | grep -Fq 'Device or resource busy' || break
  sleep 1
done
