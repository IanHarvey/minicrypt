/*
 *
 * Curve25519 (Elliptic Curve DH primitive) from Minicrypt library
 *
 * This program is placed into the public domain
 * by its author, Ian Harvey.
 * Note there is NO WARRANTY of any kind.
 */

#define MPIMINI_INTERNAL_API
#include "curve25519_mini.h"

/* ------------------ */

typedef struct
{
  F25519_Mini X;
  F25519_Mini Z;
}
  XZPoint;

static void monty_double_add( XZPoint *q, XZPoint *q_, F25519_Mini *qmqp )
{
  F25519_Mini x, z, xx, zz, xx_;
  
  F25519_add3_mini(&x, &q->X, &q->Z);
  F25519_sub3_mini(&z, &q->X, &q->Z);
  
  F25519_add3_mini(&xx, &q_->X, &q_->Z);
  F25519_sub3_mini(&zz, &q_->X, &q_->Z);
  
  F25519_mul3_mini(&xx, &xx, &z);
  F25519_mul3_mini(&zz, &x, &zz);

  F25519_add3_mini(&xx_, &xx, &zz);
  F25519_sub3_mini(&zz,  &xx, &zz);
  
  F25519_sqr_mini(&q_->X, &xx_);
  F25519_sqr_mini(&zz, &zz);
  F25519_mul3_mini(&q_->Z, &zz, qmqp);

  F25519_sqr_mini(&x, &x);
  F25519_sqr_mini(&z, &z);
  F25519_mul3_mini(&q->X, &x, &z);

  F25519_sub3_mini(&xx, &x, &z);
  F25519_mulK_mini(&z, &xx, 486662/4);
  F25519_add3_mini(&z, &z, &x);
  F25519_mul3_mini(&q->Z, &xx, &z);
}

static void F25519_modexp_inv_mini_( F25519_Mini *res, F25519_Mini *z )
{
  F25519_Mini tmp;
  int i, j;
  
  F25519_sqr_mini (&tmp, z);
  F25519_sqr_mini (&tmp, &tmp);
  F25519_mul3_mini(&tmp, &tmp, z);
  F25519_sqr_mini (&tmp, &tmp);
  F25519_mul3_mini(res, &tmp, z);
  F25519_sqr_mini (&tmp, &tmp);
  F25519_mul3_mini(&tmp, &tmp, res);

  for (i=0; i<50; i++)
  {
    for (j=0; j<5; j++)
      F25519_sqr_mini (&tmp, &tmp);
    F25519_mul3_mini(res, res, &tmp);
  }    
}

MCResult Curve25519_mini(Curve25519Msg_Mini *res, const Curve25519Msg_Mini *a,
	const Curve25519Msg_Mini *P )
{
  MCResult rc;
  int i;
  
  F25519_Mini qmqp;
  XZPoint npqp, nq;
  XZPoint *pointV[2] = { &npqp, &nq };
  
  rc = F25519_set_mini(&qmqp, P->bytes, sizeof(P->bytes));
  if ( rc != MC_OK )
    return rc;

  F25519_copy_mini(&npqp.X, &qmqp);
  F25519_setK_mini(&npqp.Z, 1);
  F25519_setK_mini(&nq.X, 1);
  F25519_setK_mini(&nq.Z, 0);
  
  monty_double_add(&npqp, &nq, &qmqp);
  
  for (i=253; i >= 3; i--)
  {
    int bit = (a->bytes[i >> 3] >> (i & 7)) & 1;
    monty_double_add( pointV[bit ^ 1], pointV[bit], &qmqp );
  }
  
  for ( ; i >= 0; i-- )
  {
    monty_double_add(&nq, &npqp, &qmqp);
  }

  
  F25519_modexp_inv_mini_(&qmqp, &nq.Z);
  F25519_mul3_mini(&nq.X, &nq.X, &qmqp);
  return F25519_get_mini(res->bytes, sizeof(res->bytes), &nq.X);
}

/* Test harness ======================================================= */
#ifdef TEST_HARNESS

#include <stdio.h>
#include <string.h>

typedef struct
{
  Curve25519Msg_Mini a;
  Curve25519Msg_Mini P;
  Curve25519Msg_Mini aP;
}
  Curve25519_TV;
  
static const Curve25519_TV curve_tvs[] =
{
#include "testvectors/curve25519.inc"
};

static const int curve_tvs_count = sizeof(curve_tvs) / sizeof(Curve25519_TV);

int main(void)
{
  int i, errs;
  
  errs = 0;
  for (i=0; i < curve_tvs_count; i++)
  {
    const Curve25519_TV *tv = &curve_tvs[i];
    Curve25519Msg_Mini res;
    
    if ( MC_OK != Curve25519_mini(&res, &tv->a, &tv->P) ||
        memcmp(&res, &tv->aP, sizeof(res)) != 0 
       )
    {
      printf("Test #%d failed\n", i);
      errs ++;
      break; //continue;
    }

  }
  
  printf("%d errors out of %d\n", errs, curve_tvs_count);
  return (errs==0) ? 0 : 1;
}

#endif /* TEST_HARNESS */

