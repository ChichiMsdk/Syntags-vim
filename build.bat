@echo off
setlocal
cd /D "%~dp0"

set HOME=%HOMEDRIVE%\%HOMEPATH%

if exist C:\vc_env\msvc\setup.bat ( @call "C:\vc_env\msvc\setup.bat")

where /q cl || (
  echo ERROR: "cl" not found - please run from MSVC x64 native tools command prompt.
  exit /b 1
)

rem -- Get command-line arguments -----------------------------------------------------
for %%a in (%*) do set "%%a=1"
if not "%msvc%"=="1" if not "%clang%"=="1" set msvc=1
if not "%debug%"=="1" set release=1
if "%debug%"=="1"   set release=0 && echo [debug mode]
if "%release%"=="1" set debug=0 && echo [release mode]
if "%msvc%"=="1"    set clang=0 && echo [msvc compile]
if "%clang%"=="1"   set msvc=0 && echo [clang compile]

rem -- CRT_NO_CRT ----------------------------------------------------------------------
set crt=0
if not "%crt%"=="1" (
  echo [NO_CRT LINKED]
  set def_crt=/DNO_CRT_LINKED
  set cc_nocrt=/Zl /Gs9999999
  set l_nocrt=/STACK:0x100000,0x100000
  set crt_libs=vcruntime.lib ucrt.lib
)
if "%crt%"=="1" (
  set def_crt=/DCRT_LINKED
  set cc_asan=/fsanitize=address
  set cc_asan=
  set cc_nocrt=
  set l_nocrt=
)

rem -- Common options -----------------------------------------------------------------
set src_dir=src
set program=highlight
set source=main
set bin=bin
set ext=.exe
set out_path=%bin%\%program%
set out_path_debug=%bin%\%program%_debug

set def_dll=
set lk_dll=

if "%ext%"==".dll" (
  set def_dll=/DCM_DLL
  set lk_dll=/DLL
)

rem -- Resource options ---------------------------------------------------------------
set assets=assets
set rc_res=%bin%\resource.res
set rsc_out=/fo %rc_res%
set rc_file=%assets%\resource.rc
set rc_includes=/i %src_dir%\
set rc=rc.exe /NOLOGO

rem -- Scaffolding ---------------------------------------------------------------------
if not exist %bin% md %bin%
if exist %out_path%.* del %out_path%.*
if exist %out_path_debug%.* del %out_path_debug%.*
if exist assets if not exist %rc_res% %rc% %rc_includes% %rsc_out% %rc_file%

rem -- MSVC compiler options ----------------------------------------------------------
set no_cpp_options=/EHsc /EHa- /GR-
set cc_secure=/GS- /guard:cf-
set cc_options=/options:strict %no_cpp_options% %cc_secure% /FC /c /Tc 
set cc_diag=/diagnostics:caret

set cc=cl /nologo %cc_diag% %cc_options%
set cc_includes=/I%src_dir% /Ichihab
set cc_def=%cc_def% %def_crt% %def_dll%

rem -- (4090)const/volatile (4189)var init unused (5045)spectre mitigation -------------
set cc_w=/wd4090 /wd4189 /wd5045 /wd4047 /wd4133 /wd4024 /wd4022 /wd4113
set cc_std=/experimental:c11atomics /std:clatest
set cc_opti=/Os /Oi
set cc_dbg=/Zi

rem -- Normal build --------------------------------------------------------------------
set cc_pdb_out=/Fd:%out_path%.pdb
set cc_obj=%out_path%.obj
set cc_out=/Fo:%cc_obj% %cc_pdb_out%

rem -- Debug build ---------------------------------------------------------------------
set cc_pdb_out_debug=/Fd:%out_path_debug%.pdb
set cc_obj_debug=%out_path_debug%.obj
set cc_out_debug=/Fo:%cc_obj_debug% %cc_pdb_out_debug%

set cc_flags=%cc_w% %cc_def% %cc_dbg% %cc_nocrt% %cc_std% %cc_opti% %cc_includes% %cc_asan%

rem -- MSVC linker options ------------------------------------------------------------
set linker=link /NOLOGO /INCREMENTAL:NO %lk_dll%
set l_debug=/DEBUG:FULL /DYNAMICBASE:NO
set l_sys=/PROFILE /GUARD:NO
set l_opt=/OPT:ICF /OPT:REF
set l_arch=/MACHINE:X64 
set l_options=/WX %l_arch% %l_nocrt% %l_sys%

set l_files=/ILK:%out_path%.ilk /MAP:%out_path%.map /PDB:%out_path%.pdb 
set l_files_debug=/ILK:%out_path_debug%.ilk /MAP:%out_path_debug%.map /PDB:%out_path_debug%.pdb 

rem -- Libraries ----------------------------------------------------------------------
rem set d3d12libs=d3dcompiler.lib D3D12.lib dxgi.lib dxguid.lib dwmapi.lib 
set win_libs=Shell32.lib uuid.lib Shlwapi.lib Ole32.lib Kernel32.lib User32.lib Gdi32.lib Credui.lib
set libs=%win_libs% %c_libs% %d3d12libs% %crt_libs% %weblibs%

set l_all=%l_debug% %l_options% %libs% %l_delay% %l_opti%
set l_out=/OUT:%out_path%%ext%
set l_out_debug=/OUT:%out_path_debug%%ext%

rem -- NORMAL BUILD -------------------------------------------------------------------
if "%release%"=="1" (
  set compiling=%cc% %src_dir%\%source%.c %cc_flags% %cc_out%
  set linking=%linker% %cc_obj% %bin%\*.res %l_files% %l_out% %l_all%
)

if "%debug%"=="1" (
  set compiling=%cc% %src_dir%\%source%.c /DDEBUG %cc_flags% %cc_out_debug%
  set linking=%linker% %cc_obj_debug% %bin%\*.res %l_files_debug% %l_out_debug% %l_all%
)

echo %compiling%
%compiling% || exit /b 1

echo:
echo %linking%
%linking%   || exit /b 1

rem -- Launch it ----------------------------------------------------------------------
rem longsure raddbg --auto-run

rem -- Misc ---------------------------------------------------------------------------
ctags -f tags --langmap=c:.c.h --languages=c -R src
