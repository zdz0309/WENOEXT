/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2011 OpenFOAM Foundation
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

Class
    Foam::WENO::globalfvMesh

Description
    Return the global mesh of the case by constructing a new mesh field if 
    it is a prallel run 

SourceFiles
    globalfvMesh.C

Author
    Jan Wilhelm Gärtner <jan.gaertner@outlook.de> Copyright (C) 2020

\*---------------------------------------------------------------------------*/

#ifndef globalfvMesh_H
#define globalfvMesh_H

#include <map>
#include <set>
#include "linear.H"
#include "processorFvPatch.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
namespace WENO
{
/*---------------------------------------------------------------------------*\
                            Class WENOBase Declaration
\*---------------------------------------------------------------------------*/

class globalfvMesh
{
    protected:
        
    // Member variables
    
        //- List of neighbour processors
        const labelList neighborProcessor_;
        
        //- List of processors that require my cells 
        //  e.g.: that have this processor as neighbour
        const labelList sendToProcessor_;
    
        //- Pointer to the global mesh
        autoPtr<fvMesh> globalMeshPtr_;
        
        const fvMesh& globalMesh_;
        
        const fvMesh& localMesh_;
        
        //- Map globalCellID to correct processor
        //  non const so it can be set within globalToLocalCellID
        labelList procList_;
        
        //- List of cellID corresponding to cellID in global mesh
        const labelList localToGlobalCellID_;
        
        //- List of mapping global cellID to localCellID
        const labelList globalToLocalCellID_;
        
    // Member functions

        //- Find closest cell and return cell index
        //  if not found return -1
        label findCellIndex(const fvMesh& mesh, const point& p);
        
    public:
    
    // Constructor
        globalfvMesh(const fvMesh& mesh);
        
    // Memeber functions 
        
        const fvMesh& localMesh() const {return localMesh_;}
        
        const fvMesh& globalMesh() const {return globalMesh_;}
        
        //- Return cellID for processor cells in global mesh
        const labelList& localToGlobalCellID() const {return localToGlobalCellID_;}
        
        //- Short hand access to globalMesh
        const fvMesh& operator()() const {return globalMesh_;}
        
        //- Is cellID a local cell
        bool isLocalCell(const int globalCellID) const;
        
        //- Get the cellID of a global cell in the requested processor
        //  Returns fatal error if not found
        int processorCellID(const int globalCellID) const;
        
        //- Get the processor of the local cell
        int getProcID(const int globalCellID) const;

};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
} // End namespace WENO
} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
