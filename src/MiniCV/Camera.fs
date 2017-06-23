﻿namespace Aardvark.Reconstruction

open System
open Aardvark.Base
open Aardvark.Reconstruction

type Camera =
    {
        location    : V3d
        forward     : V3d
        up          : V3d
        right       : V3d
    }

[<CompilationRepresentation(CompilationRepresentationFlags.ModuleSuffix)>]
module Camera =
    let private projTrafo (n : float) (f : float) : Trafo3d = 
        Trafo3d(
            M44d(
                                1.0,                     0.0,                       0.0,                        0.0,
                                0.0,                     1.0,                       0.0,                        0.0,
                                0.0,                     0.0,         (f + n) / (n - f),    (2.0 * f * n) / (n - f),
                                0.0,                     0.0,                      -1.0,                        0.0
                ),                                                     
                                                                       
            M44d(                                      
                                1.0,                     0.0,                       0.0,                       0.0,
                                0.0,                     1.0,                       0.0,                       0.0,
                                0.0,                     0.0,                       0.0,                      -1.0,
                                0.0,                     0.0,   (n - f) / (2.0 * f * n),     (f + n) / (2.0 * f * n)
                )
        )
    
    let viewTrafo (c : Camera) =
        let z = -c.forward
        let x = c.right
        let y = c.up
        let p = c.location
        Trafo3d.FromBasis(x,y,z,p).Inverse

    let viewProjTrafo1 (n : float) (f : float) (c : Camera) =
        let view = viewTrafo c
        let proj = projTrafo n f
        view * proj

    let transformed (t : Trafo3d) (c : Camera) =
        let fw = t.Forward
        {
            location = fw.TransformPos(c.location)
            forward = fw.TransformDir(c.forward)        |> Vec.normalize
            up = fw.TransformDir(c.up)                  |> Vec.normalize
            right = fw.TransformDir(c.right)            |> Vec.normalize
        }

    let transformedView (t : Trafo3d) (c : Camera) =
        let t = t.Forward
        let toWorld = M44d.FromBasis(c.right, c.up, -c.forward, c.location)
        let toRotWorld = toWorld * t
        {
            location    = toRotWorld.TransformPos(V3d.Zero)
            forward     = toRotWorld.TransformDir(-V3d.OOI) |> Vec.normalize
            up          = toRotWorld.TransformDir(V3d.OIO) |> Vec.normalize
            right       = toRotWorld.TransformDir(V3d.IOO) |> Vec.normalize
        }
       
    let project1 (c : Camera) (pt : V3d) =
        let o = pt - c.location
        let pc = 
            V3d(
                Vec.dot o c.right,
                Vec.dot o c.up,
                Vec.dot o -c.forward
            )

        pc.XY / pc.Z

    let unproject1 (c : Camera) (pt : V2d) =
        let pc = V3d(pt, 1.0)
        let direction =
            pc.X * c.right +
            pc.Y * c.up +
            pc.Z * -c.forward

        Ray3d(c.location, Vec.normalize direction)

    let lookAt (eye : V3d) (center : V3d) (sky : V3d) =
        let fw = center - eye       |> Vec.normalize
        let r = Vec.cross fw sky    |> Vec.normalize
        let u = Vec.cross r fw      |> Vec.normalize

        { 
            location = eye
            forward = fw
            up = u
            right = r
        }

    let angles (l : Camera) (r : Camera) =
        let af = acos (Vec.dot l.forward r.forward |> clamp -1.0 1.0) |> abs
        let ar = acos (Vec.dot l.right r.right |> clamp -1.0 1.0) |> abs
        let au = acos (Vec.dot l.up r.up |> clamp -1.0 1.0) |> abs
        V3d(ar, au, af)

    let distance (l : Camera) (r : Camera) =    
        Vec.length (l.location - r.location)

    let approxEqual (angleTol : float) (spatialTol : float) (l : Camera) (r : Camera) =
        
        let dl = Vec.length (l.location - r.location) |> abs
        let af = acos (Vec.dot l.forward r.forward |> clamp -1.0 1.0) |> abs
        let ar = acos (Vec.dot l.right r.right |> clamp -1.0 1.0) |> abs
        let au = acos (Vec.dot l.up r.up |> clamp -1.0 1.0) |> abs
        dl <= spatialTol && af <= angleTol && ar <= angleTol && au <= angleTol