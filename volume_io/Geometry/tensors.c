#include  <internal_volume_io.h>

private  void  multiply_basis_matrices(
    int    n_derivs,
    int    n_degs,
    Real   m1[],
    Real   m2[],
    Real   prod[] )
{
    int   i, j, k, m2_inc;
    Real  *m1_ptr, *m1_ptr1;
    Real  *m2_ptr;
    Real  sum, *prod_ptr;

    m1_ptr = m1;
    prod_ptr = prod;
    m2_inc = 1 - n_degs * n_degs;
    for_less( i, 0, n_derivs )
    {
        m2_ptr = m2;
        for_less( j, 0, n_degs )
        {
            sum = 0.0;
            m1_ptr1 = m1_ptr;
            for_less( k, 0, n_degs )
            {
                sum += (*m1_ptr1) * (*m2_ptr);
                ++m1_ptr1;
                m2_ptr += n_degs;
            }
            m2_ptr += m2_inc;
            *prod_ptr = sum;
            ++prod_ptr;
        }

        m1_ptr += n_degs;
    }
}

private  void  multiply_matrices(
    int    x1,
    int    y1,
    Real   m1[],
    int    sa1,
    int    sb1,
    int    y2,
    Real   m2[],
    int    sa2,
    int    sb2,
    Real   prod[],
    int    sap,
    int    sbp )
{
    int   i, j, k;
    Real  *m1_ptr, *m1_ptr1, *m2_ptr;
    Real  sum, *prod_ptr;

    m1_ptr = m1;
    prod_ptr = prod;
    sb2 -= y1 * sa2;
    sap -= y2 * sbp;
    for_less( i, 0, x1 )
    {
        m2_ptr = m2;
        for_less( j, 0, y2 )
        {
            sum = 0.0;
            m1_ptr1 = m1_ptr;
            for_less( k, 0, y1 )
            {
                sum += (*m1_ptr1) * (*m2_ptr);
                m1_ptr1 += sb1;
                m2_ptr += sa2;
            }

            *prod_ptr = sum;

            prod_ptr += sbp;
            m2_ptr += sb2;
        }
        m1_ptr += sa1;
        prod_ptr += sap;
    }
}

#define  MAX_DEGREE        4
#define  MAX_DIMS          10
#define  MAX_TOTAL_VALUES  4000

public  void  spline_tensor_product(
    int     n_dims,
    Real    positions[],
    int     degrees[],     /* [n_dims] */
    Real    *bases[],      /* [n_dims][degress[dim]*degrees[dim]] */
    int     n_values,
    Real    coefs[],       /* [n_values*degrees[0]*degrees[1]*...] */
    int     n_derivs[],    /* [n_dims] */
    Real    results[] )    /* [n_values*n_derivs[0]*n_derivs[1]*...] */
{
    int       deriv, d, k, total_values, src;
    int       ind, prev_ind, max_degree, n_derivs_plus_1, deg;
    int       static_indices[MAX_DIMS];
    int       *indices, total_derivs;
    Real      *input_coefs, u_power, u;
    Real      static_us[MAX_DEGREE*MAX_DEGREE];
    Real      static_weights[MAX_DEGREE*MAX_DEGREE];
    Real      *us, *weights;
    Real      *tmp_results[2], *r;
    Real      static_tmp_results[2][MAX_TOTAL_VALUES];
    BOOLEAN   results_alloced;

    /*--- check arguments */

    max_degree = 2;
    total_values = n_values;
    total_derivs = 0;
    for_less( d, 0, n_dims )
    {
        if( degrees[d] < 2  )
        {
            print( "spline_tensor_product: Degree %d must be greater than 1.\n",
                   degrees[d] );
            return;
        }
        if( degrees[d] > max_degree )
            max_degree = degrees[d];
        if( n_derivs[d] > total_derivs )
            total_derivs = n_derivs[d];

        total_values *= degrees[d];
    }

    if( n_dims > MAX_DIMS )
    {
        ALLOC( indices, n_dims );
    }
    else
    {
        indices = static_indices;
    }

    if( max_degree > MAX_DEGREE )
    {
        ALLOC( us, max_degree * max_degree );
        ALLOC( weights, max_degree * max_degree );
    }
    else
    {
        us = static_us;
        weights = static_weights;
    }

    if( total_values > MAX_TOTAL_VALUES )
    {
        ALLOC( tmp_results[0], total_values );
        ALLOC( tmp_results[1], total_values );
        results_alloced = TRUE;
    }
    else
    {
        tmp_results[0] = static_tmp_results[0];
        tmp_results[1] = static_tmp_results[1];
        results_alloced = FALSE;
    }

    input_coefs = coefs;

    src = 0;

    /*--- do each dimension */

    for_less( d, 0, n_dims )
    {
        deg = degrees[d];
        n_derivs_plus_1 = 1 + n_derivs[d];

        u = positions[d];
        u_power = 1.0;
        us[0] = 1.0;
        for_less( k, 1, deg )
        {
            u_power *= u;
            us[k] = u_power;
        }

        ind = deg;
        for_less( deriv, 1, n_derivs_plus_1 )
        {
            for_less( k, 0, deriv )
            {
                us[ind] = 0.0;
                ++ind;
            }
   
            prev_ind = IJ( deriv-1, deriv-1, deg );
            for_less( k, deriv, deg )
            {
                us[ind] = us[prev_ind] * (Real) k;
                ++ind;
                ++prev_ind;
            }
        }

        multiply_basis_matrices( n_derivs_plus_1, deg, us, bases[d], weights );

        total_values /= deg;

        if( d == n_dims-1 )
            r = results;
        else
            r = tmp_results[1-src];

        multiply_matrices( n_derivs_plus_1, deg, weights, deg, 1,
                           total_values, input_coefs, total_values, 1,
                           r, 1, n_derivs_plus_1 );

        src = 1 - src;
        input_coefs = tmp_results[src];

        total_values *= n_derivs_plus_1;
    }

    if( n_dims > MAX_DIMS )
    {
        FREE( indices );
    }

    if( max_degree > MAX_DEGREE )
    {
        FREE( us );
        FREE( weights );
    }

    if( results_alloced )
    {
        FREE( tmp_results[0] );
        FREE( tmp_results[1] );
    }
}