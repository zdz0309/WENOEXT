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
    Jan Wilhelm Gärtner <jan.gaertner@outlook.de> Copyright (C) 2020

\*---------------------------------------------------------------------------*/

#include "matrixDB.H"
#include <stdint.h>
#include <inttypes.h>
// * * * * * * * * * * *  ScalarRectangularMatrixPtr * * * * * * * * * * * * //

Foam::matrixDB::scalarRectangularMatrixPtr::scalarRectangularMatrixPtr(matrixDB* db)
: 
    matrixDB_(db),
    itr_(db->DB_.end())
{}


void Foam::matrixDB::scalarRectangularMatrixPtr::add
(
    const scalarRectangularMatrix&& A
)
{
    // search the databank for a similar matrix and return iterator
    itr_ = matrixDB_->similar(std::move(A));
}


const Foam::scalarRectangularMatrix& 
Foam::matrixDB::scalarRectangularMatrixPtr::operator()() const
{
    #ifdef FULLDEBUG
        if (!this->valid())
            FatalErrorInFunction()
                << "Access non valid Iterator" << exit(FatalError);
    #endif
    
    return itr_->second;
}


bool Foam::matrixDB::scalarRectangularMatrixPtr::valid() const
{
    if ((matrixDB_ != nullptr) && (itr_ != matrixDB_->DB_.end()))
        return true;

    return false;
}


// * * * * * * * * * * * * * * * matrixDB  * * * * * * * * * * * * * * * * * //

std::multimap<int32_t,Foam::scalarRectangularMatrix>::const_iterator
Foam::matrixDB::similar
(
    const scalarRectangularMatrix&& A
)
{
    int32_t key = hashMatrix(A);
    
    if (DB_.size() == 0)
    {
        auto it = DB_.emplace(key,A);
        return it;
    }
    

    auto maxMag = [] (const scalarRectangularMatrix& A) -> double
    {
        double maxA = 0;
        for (int i = 0; i < A.m(); i++)
        {
            for (int j = 0; j < A.n(); j++)
            {
                if (mag(A[i][j]) > maxA)
                    maxA = mag(A[i][j]);
            }
        }
        return maxA;
    };
    // Calculate tolerance
    double tol = epsilon_ * maxMag(A);


    
    // Search the next neighbours
    auto itStart = DB_.lower_bound(key-checkRange_);
    auto itEnd = DB_.lower_bound(key+checkRange_);
    for (auto it = itStart; it != itEnd;it++)
    {
        const scalarRectangularMatrix& cmpA = it->second;
        
        bool validEntry = true;
        if (cmpA.size() == A.size())
        {
            for (int i = 0; i < A.m(); i++)
            {
                for (int j = 0; j < A.n(); j++)
                {
					if (mag(A[i][j]) < SMALL)
						continue;
                    if (mag((cmpA[i][j] - A[i][j])) > tol)
                    {
                        validEntry = false;
                        break;
                    }
                }
                if (validEntry == false)
                    break;
            }
            
            if (validEntry)
            {
                counter_++;
                return it;    
            }
        } 
    }
    

    // Use insert with hint 
    auto it = DB_.insert
    (
        std::pair<int32_t,scalarRectangularMatrix>(key,std::move(A))
    );
    return it;    
}


int32_t Foam::matrixDB::hashMatrix
(
    const scalarRectangularMatrix& A
)
{
    auto maxMag = [] (const scalarRectangularMatrix& A) -> double
    {
        double maxA = 0;
        for (int i = 0; i < A.m(); i++)
        {
            for (int j = 0; j < A.n(); j++)
            {
                if (mag(A[i][j]) > maxA)
                    maxA = mag(A[i][j]);
            }
        }
        return maxA;
    };

    int32_t key = 0;
    
    double mult = 1.0E+6/maxMag(A);
    for (int i = 0; i < A.m(); i++)
    {
        for (int j = 0; j < A.n(); j++)
        {
            key += int32_t(A[i][j] *mult);
        }
    }
    return key; 
    
    /******************** Hash Algorithm Java *********************************\
    //auto doubleToRawBits = [](double x) -> uint64_t
    //{
        //uint64_t bits;
        //memcpy(&bits, &x, sizeof bits);
        //return bits;
    //};
    
    //// Hash function of Java Arrays.hashCode 
    //// (http://developer.classpath.org/doc/java/util/Arrays-source.html)
    //int32_t key = 1;
    //for (int i = 0; i < A.m(); i++)
    //{
        //for (int j = 0; j < A.n(); j++)
        //{
            //uint64_t l = doubleToRawBits(A[i][j]);
            //int32_t elt = static_cast<int32_t>(l ^ (l >> 32));
            //key = 31*key + elt;
        //}
    //}
    //return key;
    \**************************************************************************/
}


const Foam::List<Foam::matrixDB::scalarRectangularMatrixPtr>& 
Foam::matrixDB::operator[](const label celli) const
{
    return LSmatrix_[celli];
}


Foam::List<Foam::matrixDB::scalarRectangularMatrixPtr>& 
Foam::matrixDB::operator[](const label celli)
{
    return LSmatrix_[celli];
}


void Foam::matrixDB::resizeSubList(const label cellI, const label size)
{
    LSmatrix_[cellI].resize(size,scalarRectangularMatrixPtr(this));
}


void Foam::matrixDB::info()
{
    scalar numElements = 0;
    forAll(LSmatrix_,celli)
    {
        forAll(LSmatrix_[celli],stencilI)
        {
            if (LSmatrix_[celli][stencilI].valid())
                numElements++;
        }
    }
    
    Pout << "\tMatrix Database Statistics: "<<nl
         << "\t\tTotal Number of matrices: "<< numElements << nl
         << "\t\tNumber matrices stored: "<<DB_.size() <<nl
         << "\t\tMemory reduction:       "<<100.0 - double(DB_.size())/numElements*100.0<<nl
         << "\t\tCounter: "<<counter_<< endl;
}


void Foam::matrixDB::write(Ostream& os) const
{    
    os << LSmatrix_.size()<<endl;
    forAll(LSmatrix_,cellI)
    {
        os << LSmatrix_[cellI].size()<<endl;
        forAll(LSmatrix_[cellI],stencilI)
        {
            os << LSmatrix_[cellI][stencilI].iterator()->first<<endl;
            os << LSmatrix_[cellI][stencilI].iterator()->second<<endl;
        }
    }
}


void Foam::matrixDB::read(Istream& is)
{
    scalarRectangularMatrix matrix;
    scalar key;
    // Read in the LSMatrix list
    label size;
    is >> size;
    LSmatrix_.resize(size);
    
    forAll(LSmatrix_,cellI)
    {
        is >> size;
        LSmatrix_[cellI].resize(size,scalarRectangularMatrixPtr(this));
        
        forAll(LSmatrix_[cellI],stencilI)
        {
            is >> key;
            is >> matrix;
            LSmatrix_[cellI][stencilI].add(std::move(matrix));
        }
    }
}


Foam::Istream& Foam::operator >>(Istream& is, matrixDB& matrixDB_)
{
    matrixDB_.read(is);
    return is;
}


Foam::Ostream& Foam::operator <<(Ostream& os,const matrixDB& matrixDB_)
{
    matrixDB_.write(os);
    return os;
}
