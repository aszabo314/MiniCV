#load @"paket-files/build/vrvis/Aardvark.Fake/DefaultSetup.fsx"

open Fake
open System
open System.IO
open System.Diagnostics
open Aardvark.Fake

do Environment.CurrentDirectory <- __SOURCE_DIRECTORY__

DefaultSetup.install ["src/MiniCV.sln"]

Target "Run" (fun() ->
    tracefn "exec: %d" (Shell.Exec "bin/Release/MiniCV.exe")
)

Target "Test" (fun () ->
    Fake.NUnitSequential.NUnit (fun p -> { p with ToolPath = @"packages\NUnit.Runners\tools"
                                                  ToolName = "nunit-console.exe" }) [@"bin\Release\Aardvark.Base.Incremental.Tests.exe"]
)

Target "CopyOpenCVDebug" (fun () ->
    CopyFile "bin/Debug" "lib/opencv_world320d.dll"
)

"CopyOpenCVDebug" ==> "AddNativeResources"



Target "Statistics" (fun () ->
    let fsFiles = !!"src/**/*.fs"  

    let mutable stats = Map.empty
    for f in fsFiles do
        tracefn "file: %A" f
        ()
)

entry()