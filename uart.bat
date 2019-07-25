echo off
c6805 uart.c +o +l +e
pause
bclink uart
copy uart.map uart.m
copy uart.sym uart.map
