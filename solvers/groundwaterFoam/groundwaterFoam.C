/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2011-2016 OpenFOAM Foundation
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

Application
    groundwaterFoam

Description
    Transient solver for Richards equation. 
    A Picard loop is used for linearization.
    Permeability is isotropic (K == volScalarField)

Developers
    P. Horgue, J. Franc, R. Guibert and G. Debenest

\*---------------------------------------------------------------------------*/

#include "fvCFD.H"
#include "harmonic.H"
#include "incompressiblePhase.H"
#include "capillarityModel.H"
#include "relativePermeabilityModel.H"
#include "fixedValueFvPatchField.H"
#include "sourceEventFile.H"
#include "outputEventFile.H"
#include "patchEventFile.H"
#include "eventInfiltration.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
using namespace Foam;

int main(int argc, char *argv[])
{
    #include "setRootCase.H"
    #include "createTime.H"
    #include "createMesh.H"
    #include "readGravitationalAcceleration.H"
    #include "createFields.H"
    #include "createthetaFields.H"
    #include "readPicardControls.H"
    #include "readTimeControls.H"
    #include "readEvent.H"
    #include "readForcing.H"

    // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

    Info<< "\nStarting time loop\n" << endl;
    label iterPicard=0;

    while (runTime.run())
    {
        if (outputEventIsPresent) outputEvent.updateIndex(runTime.timeOutputValue());
        if (sourceEventIsPresent) sourceEvent.updateIndex(runTime.timeOutputValue());
        forAll(patchEventList,patchEventi) patchEventList[patchEventi]->updateIndex(runTime.timeOutputValue());
        #include "setDeltaT.H"

        runTime++;

noConvergence :
        Info << "Time = " << runTime.timeName() << nl << endl;

        #include "computeSourceTerm.H"
        scalar resPicard=GREAT;
        iterPicard = 0;
        while ((resPicard > tolPicard) && (iterPicard != maxIterPicard))
        {
            iterPicard++;
            #include "hEqn.H"
            #include "updateProperties.H"
        }
        if (resPicard > tolPicard)
        {
            Info << endl;
            Warning() <<  " Max iteration reached in Picard loop, reducing time step by factor dTFactDecrease" << nl << endl;
            iterPicard++;
            h = h.oldTime();
            //- rewind time
            runTime.setTime(runTime.timeOutputValue()-runTime.deltaTValue(),runTime.timeIndex());
            //- recompute time step
            #include "setDeltaT.H"
            //- Update new time
            runTime.setTime(runTime.timeOutputValue()+runTime.deltaTValue(),runTime.timeIndex());
            #include "updateProperties.H"
            goto noConvergence;
        }

        Info << "Saturation theta " << " Min(theta) = " << gMin(theta.internalField()) << " Max(theta) = " << gMax(theta.internalField()) <<  endl;
        Info << "Head pressure h  " << " Min(h) = " << gMin(h.internalField()) << " Max(h) = " << gMax(h.internalField()) <<  endl;
        volScalarField dtheta_tmp(mag(theta-theta.oldTime()));
        dtheta = gMax(dtheta_tmp);
        dthetadTmax = dtheta/runTime.deltaTValue();
        dtheta_avg = dtheta_tmp.weightedAverage(mesh.V()).value();

        #include "waterMassBalance.H"
        #include "eventWrite.H"

        Info<< "ExecutionTime = " << runTime.elapsedCpuTime() << " s"
            << "  ClockTime = " << runTime.elapsedClockTime() << " s"
            << nl << endl;
    }

    Info<< "End\n" << endl;

    return 0;
}

// ************************************************************************* //
