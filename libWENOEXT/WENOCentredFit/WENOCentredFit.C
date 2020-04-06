/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2011-2013 OpenFOAM Foundation
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

Author
    Tobias Martin, <tobimartin2@googlemail.com>.  All rights reserved.
    Jan Wilhelm Gärtner, <jan.gaertner@outlook.de>
    
\*---------------------------------------------------------------------------*/

#include "codeRules.H"
#include "WENOCoeff.H"
#include "WENOCentredFit.H"
#include "processorFvPatch.H"
#include "cyclicFvPatch.H"

// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

template<class Type>
Foam::tmp<Foam::GeometricField<Type, Foam::fvsPatchField, Foam::surfaceMesh> >
Foam::WENOCentredFit<Type>::correction
(
    const GeometricField<Type, fvPatchField, volMesh>& vf
) const
{
    const fvMesh& mesh = this->mesh();

    // Get degrees of freedom from WENOCoeff class
    tmp<Field<Field<Type> > > coeffsWeightedTmp = WENOCoeff_.getWENOPol(vf);
    const Field<Field<Type> >& coeffsWeighted = coeffsWeightedTmp(); 

    // Calculate the interpolated face values
    
    const labelUList& P = mesh.owner();
    const labelUList& N = mesh.neighbour();

    tmp<GeometricField<Type, fvsPatchField, surfaceMesh> > tsfCorr
    (
        new GeometricField<Type, fvsPatchField, surfaceMesh>
        (
            IOobject
            (
                "tvf",
                mesh.time().timeName(),
                mesh,
                IOobject::NO_READ,
                IOobject::NO_WRITE,
                false
            ),
            mesh,
            dimensioned<Type>(vf.name(), vf.dimensions(), pTraits<Type>::zero)
        )
    );
    GeometricField<Type, fvsPatchField, surfaceMesh>& tsf =
    #ifdef FOAM_NEW_TMP_RULES
        tsfCorr.ref();
    #else 
        tsfCorr();
    #endif
    
    const surfaceScalarField& weights = mesh.surfaceInterpolation::weights();        
    
    // Linear combination of correction polynomials
        
    forAll(P, faceI)    
    {                
        Type WENOcontribution =
            sumFlux
            (
                WENOBase_.dimList()[P[faceI]],
                coeffsWeighted[P[faceI]],
                WENOBase_.intBasTrans()[faceI][0]
            ) / WENOBase_.refFacAr()[faceI];

        tsf[faceI] = 
            (1.0-weights[faceI])*(vf[P[faceI]] - vf[N[faceI]])
            + WENOcontribution;
    }
    
    coupledRiemannSolver(mesh, tsf, vf, weights, coeffsWeighted);        

    return tsfCorr;        
} 


template<class Type>
Type Foam::WENOCentredFit<Type>::sumFlux
(        
    const labelList& dim,
    const Field<Type>& coeffcI,
    const volIntegralType& intBasiscIfI
)    const
{
    Type flux = pTraits<Type>::zero;

    label nCoeff = 0;

    for (label n = 0; n <= dim[0]; n++)
    {
        for (label m = 0; m <= dim[1]; m++)
        {
            for (label l = 0; l <= dim[2]; l++)
            {
                if ((n+m+l) <= polOrder_ && (n+m+l) > 0)
                {                
                    flux += 
                        coeffcI[nCoeff]*intBasiscIfI[n][m][l];
                        
                    nCoeff++;                                                    
                }
            }
        }
    }    
        
    return flux; 
}    


template<class Type>
void Foam::WENOCentredFit<Type>::coupledRiemannSolver
(
    const fvMesh& mesh,
    GeometricField<Type, fvsPatchField, surfaceMesh>& tsf,
    const GeometricField<Type, fvPatchField, volMesh>& vf,
    const surfaceScalarField& weights,
    const Field<Field<Type> >& coeffsWeighted
)   const
{
    const fvPatchList& patches = mesh.boundary();
    
    const typename GeometricField<Type, fvPatchField, volMesh>::
    #ifdef FOAM_NEW_GEOMFIELD_RULES
        Boundary& bvf = vf.boundaryField();
    #else 
        GeometricBoundaryField& bvf = vf.boundaryField();
    #endif           
        
    typename GeometricField<Type, fvsPatchField, surfaceMesh>::
    #ifdef FOAM_NEW_GEOMFIELD_RULES
        Boundary& btsf = tsf.boundaryFieldRef();
    #else 
        GeometricBoundaryField& btsf = tsf.boundaryField();
    #endif


    forAll(btsf, patchI)
    {
        fvsPatchField<Type>& pSfCorr = btsf[patchI];                    
    
        if (patches[patchI].coupled())
        {
            tmp<Field<Type>> patchNeighbourField  = bvf[patchI].patchNeighbourField();
            
            const labelUList& pOwner = mesh.boundary()[patchI].faceCells();    
                
            label startFace = patches[patchI].start();
        
            forAll(pOwner, faceI)
            {                
                label own = pOwner[faceI];
                    
                pSfCorr[faceI] = (1.0 - weights.boundaryField()[patchI][faceI])
                                 *(vf[own]-patchNeighbourField()[faceI]);
                    
                pSfCorr[faceI] += 
                    sumFlux
                        (
                            WENOBase_.dimList()[own],
                            coeffsWeighted[own],
                            WENOBase_.intBasTrans()[faceI + startFace][0]
                        )  /WENOBase_.refFacAr()[faceI + startFace];
            }
        }
    }
}


// ************************************************************************* //