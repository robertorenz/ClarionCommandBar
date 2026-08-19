@echo off
rem =====================================================================
rem install.bat - put ClaCommandBar where Clarion's redirection looks,
rem               and register the template.
rem
rem   ClaCommandBar.tpl -> accessory\template\win
rem   CommandBar.inc    -> accessory\libsrc\win
rem   CommandBar.clw    -> accessory\libsrc\win
rem   commandbar.lib    -> accessory\lib AND accessory\libsrc\win
rem   commandbar.dll    -> accessory\bin
rem
rem Those five folders are on the search paths in bin\CLARION120.RED, so
rem after this ANY app on the machine finds the class, links the lib and
rem has commandbar.dll copied into its output folder automatically.
rem
rem Run it again after rebuilding the engine (src\build.bat): the import
rem library binds BY ORDINAL, so an app built against a newer lib that
rem loads the older DLL out of accessory\bin fails with
rem "Entry Point Not Found".
rem
rem   install.bat                 uses C:\clarion12
rem   install.bat D:\Clarion12    uses that instead
rem =====================================================================
setlocal
set CLA=%~1
if "%CLA%"=="" set CLA=C:\clarion12

if not exist "%CLA%\bin\ClarionCL.exe" (
  echo.
  echo Cannot find "%CLA%\bin\ClarionCL.exe".
  echo Pass your Clarion folder:  install.bat D:\Clarion12
  exit /b 1
)

cd /d "%~dp0"
if not exist ClaCommandBar.tpl goto :nosrc
if not exist CommandBar.inc    goto :nosrc
if not exist ..\bin\commandbar.dll goto :nodll

rem  Back the template registry up first.  A bad registration is only
rem  undone by restoring this file - see INSTALL.md section 4.
if exist "%CLA%\template\win\TemplateRegistry12.trf" (
  echo --- backing up the template registry ---
  copy /y "%CLA%\template\win\TemplateRegistry12.trf" ^
          "%CLA%\template\win\TemplateRegistry12.trf.bak" >nul
)

echo --- copying ---
if not exist "%CLA%\accessory\template\win" mkdir "%CLA%\accessory\template\win"
if not exist "%CLA%\accessory\libsrc\win"   mkdir "%CLA%\accessory\libsrc\win"
if not exist "%CLA%\accessory\lib"          mkdir "%CLA%\accessory\lib"
if not exist "%CLA%\accessory\bin"          mkdir "%CLA%\accessory\bin"

copy /y ClaCommandBar.tpl     "%CLA%\accessory\template\win\" >nul || goto :failed
copy /y CommandBar.inc        "%CLA%\accessory\libsrc\win\"   >nul || goto :failed
copy /y CommandBar.clw        "%CLA%\accessory\libsrc\win\"   >nul || goto :failed
copy /y commandbar.lib        "%CLA%\accessory\lib\"          >nul || goto :failed
rem  ALSO into libsrc\win.  A copy that lands there shadows the one in
rem  accessory\lib - the linker finds it first - so an old one left behind
rem  makes new exports come back as "Unresolved External" however many
rem  times you rebuild.  Refreshing both keeps that from happening.
copy /y commandbar.lib        "%CLA%\accessory\libsrc\win\"   >nul || goto :failed
copy /y ..\bin\commandbar.dll "%CLA%\accessory\bin\"          >nul || goto :failed

echo --- registering the template ---
rem  Registered from its INSTALLED path: the registry keeps the source
rem  file name and re-reads it every time an app is opened, and a name
rem  on the redirection path always resolves.
pushd "%CLA%"
"%CLA%\bin\ClarionCL.exe" -tr "%CLA%\accessory\template\win\ClaCommandBar.tpl"
popd

echo.
echo Installed into %CLA%
echo Close and reopen the Clarion IDE - it caches a PARSED copy of the
echo template, so an IDE that was already open keeps serving the old one.
endlocal & exit /b 0

:nosrc
echo Run this from the clarion\ folder of the repository.
endlocal & exit /b 1

:nodll
echo ..\bin\commandbar.dll is missing - build it with ..\src\build.bat
endlocal & exit /b 1

:failed
echo.
echo A copy failed.  Close the Clarion IDE and any running app that has
echo commandbar.dll loaded, then try again.
endlocal & exit /b 1
