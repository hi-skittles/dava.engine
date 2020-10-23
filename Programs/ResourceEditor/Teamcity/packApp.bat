cd ..\Release

if exist app rmdir /s /q app
if exist app.zip del /q app.zip

mkdir app\1\2\3\Data
mkdir app\dava.resourceeditor.beast
xcopy /e ..\Data\*.* app\1\2\3\Data 
xcopy *.exe app\1\2\3
xcopy ..\glew32.dll app\1\2\3
xcopy ..\..\..\..\dava.resourceeditor.beast\beast\bin\beast32.dll app\1\2\3
xcopy /e ..\..\..\..\dava.resourceeditor.beast\*.* app\dava.resourceeditor.beast

wzzip -p -r ResourceEditor_win.zip app\*.*	