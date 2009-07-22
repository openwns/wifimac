/** -*- c++ -*- \file CholeskyDecomposition.hpp \brief cholesky decomposition */
/*
 -   begin                : 2005-08-24
 -   copyright            : (C) 2005 by Gunter Winkler, Konstantin Kutzkow
 -   email                : guwi17@gmx.de

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/

#ifndef _H_CHOLESKY_HPP_
#define _H_CHOLESKY_HPP_

#include <cassert>

#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/vector_proxy.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/matrix_proxy.hpp>
#include <boost/numeric/ublas/vector_expression.hpp>
#include <boost/numeric/ublas/matrix_expression.hpp>
#include <boost/numeric/ublas/triangular.hpp>

using namespace boost::numeric::ublas;

template < class MATRIX >
size_t choleskyDecompose(MATRIX& A)
{
    typedef typename MATRIX::value_type T;
    const MATRIX& A_c(A);
    const size_t n = A.size1();

    for (size_t k=0 ; k < n; k++)
    {
        double qL_kk = A_c(k,k) - inner_prod( project( row(A_c, k), range(0, k) ),
                                              project( row(A_c, k), range(0, k) ) );

        if (qL_kk <= 0)
        {
            return 1 + k;
        }
        else
        {
            double L_kk = sqrt( qL_kk );

            matrix_column<MATRIX> cLk(A, k);
            project( cLk, range(k+1, n) )
                = ( project( column(A_c, k), range(k+1, n) )
                    - prod( project(A_c, range(k+1, n), range(0, k)),
                            project(row(A_c, k), range(0, k) ) ) ) / L_kk;
            A(k,k) = L_kk;
        }
    }
    return 0;
}

#endif
