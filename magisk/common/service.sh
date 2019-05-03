#!/system/bin/sh
while true; do
  magiskpolicy --live 'allow bootanim kernel system syslog_read' 2>&1 |
  grep -Fq 'Device or resource busy' || break
  sleep 1
done
