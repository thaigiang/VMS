del *.hex
del *.map
mkdir "Objects - Internet Bootloader App"

set Processor=18F97J60

:SelectProcessor
@IF EXIST "Internet Bootloader for PIC18F97J60 Extended Mode.hex" set Processor=18F96J65
@IF EXIST "Internet Bootloader for PIC18F96J65 Extended Mode.hex" set Processor=18F96J60
@IF EXIST "Internet Bootloader for PIC18F96J60 Extended Mode.hex" set Processor=18F87J60
@IF EXIST "Internet Bootloader for PIC18F87J60 Extended Mode.hex" set Processor=18F86J65
@IF EXIST "Internet Bootloader for PIC18F86J65 Extended Mode.hex" set Processor=18F86J60
@IF EXIST "Internet Bootloader for PIC18F86J60 Extended Mode.hex" set Processor=18F67J60
@IF EXIST "Internet Bootloader for PIC18F67J60 Extended Mode.hex" set Processor=18F66J65
@IF EXIST "Internet Bootloader for PIC18F66J65 Extended Mode.hex" set Processor=18F66J60
@IF EXIST "Internet Bootloader for PIC18F66J60 Extended Mode.hex" GOTO EOF

@echo ####################################
@echo # Building bootloader for %Processor%
@echo ####################################
@del ".\Objects - Internet Bootloader App\*.*" /q
@"C:\MCC18\bin\mcc18.exe" -p=%Processor%  /i"." -I"..\Include" -I"..\Microchip\Include" -I"..\..\Internet Bootloader App" -I"C:\MCC18\h" "Main.c" -fo="Objects - Internet Bootloader App\Main.o" -k -sco -w3 -mL -Ou- -Ot- -Ob- -Op- -Or- -Od- -Opa-
@"C:\MCC18\bin\mplink.exe" /l"C:\MCC18\lib" ".\Linker\%processor%i_bootloader.lkr" ".\Objects - Internet Bootloader App\Main.o" /m"Internet Bootloader.map" /w /o"Internet Bootloader.cof"
@ren "Internet Bootloader.hex" "Internet Bootloader for PIC%processor%.hex"
@ren "Internet Bootloader.map" "Internet Bootloader for PIC%processor%.map"
@"C:\MCC18\bin\mcc18.exe" -p=%Processor%  /i"." -I"..\Include" -I"..\Microchip\Include" -I"..\..\Internet Bootloader App" -I"C:\MCC18\h" "Main.c" -fo="Objects - Internet Bootloader App\Main.o" --extended -k -w3 -mL -Ou- -Ot- -Ob- -Op- -Or- -Od- -Opa-
@"C:\MCC18\bin\mplink.exe" /l"C:\MCC18\lib" ".\Linker\%processor%i_e_bootloader.lkr" ".\Objects - Internet Bootloader App\Main.o" /m"Internet Bootloader.map" /w /o"Internet Bootloader.cof"
@ren "Internet Bootloader.hex" "Internet Bootloader for PIC%processor% Extended Mode.hex"
@ren "Internet Bootloader.map" "Internet Bootloader for PIC%processor% Extended Mode.map"
@GOTO SelectProcessor

:EOF