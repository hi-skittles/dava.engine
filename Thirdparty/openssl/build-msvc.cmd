rem First arg - build type x86 or x64
rem Output libs will be always in out32 dir, includes in inc32

set _PLATFORM=x86
set _COMPILER=VC-WIN32
set OPTS=no-whirlpool no-asm no-cast no-idea no-camellia no-hw 

if not [%1] == [x64] goto run

set _PLATFORM=x64
set _COMPILER=VC-WIN64A

:run

perl Configure %OPTS% %_COMPILER%
perl util\mkfiles.pl >MINFO

perl util\mk1mf.pl %OPTS% %_COMPILER% >ms\nt.mak
perl util\mk1mf.pl %OPTS% debug %_COMPILER% >ms\ntd.mak

nmake -f ms\nt.mak
nmake -f ms\ntd.mak
