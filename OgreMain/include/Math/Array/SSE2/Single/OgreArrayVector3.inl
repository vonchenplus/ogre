/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2013 Torus Knot Software Ltd

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/

namespace Ogre
{
	/** HOW THIS WORKS:

		As you may have been explained in OgreArrayVector3.h, ArraVector3 uses heap memory,
		but operators can produce intermediate results that will live in stack memory, hence
		ArrayVector3 comes into play.
		The problem is that doing a = a + b, either a or b could intermediate vectors or
		real arrayvectors. Hence we need the operators:
		+ ( ArrayVector3, ArrayVector3 );		// -> Both are array vectors
		+ ( ArrayVector3, ArrayVector3 );		// -> Both are intermediate vectors
		+ ( ArrayVector3, ArrayInterVector );	// -> Mix (ver. 1)
		+ ( ArrayInterVector, ArrayVector3 );	// -> Mix (ver. 2)

		And repeat for every function & operator we've got. Some have to be written twice
		(i.e. those taking one argument), some have to be written 4 times (two arguments)
		In other words the number of variations is 2^args

		In some cases we want to add scalars, so we also need the operators:
		+ ( ArrayVector3, float scalar );		// -> a * 2.0f
		+ ( float scalar, ArrayVector3 );		// -> 2.0f * a
		+ ( ArrayInterVector, float scalar );	// -> a * 2.0f, a is intermediate
		+ ( float scalar, ArrayInterVector );	// -> 2.0f * a, a is intermediate

		Also consider that ArrayReal is a scalar but at vector level (not array level) i.e.
		a * b.dotProduct( c ) -> where the result of b.dot( c ) is a ArrayReal scalar, then
		we need another 4 '+' operators.
		In total we would need 12 '+' operators where the code is almost identical.

		Instead of writing 12 times the same code, we use macros:
		DEFINE_OPERATION( ArrayVector3, ArrayVector3, +, _mm_add_ps );
		Means "define '+' operator that takes both arguments as ArrayVector3 and use
		the _mm_add_ps intrinsic to do the job"

		Note that for scalars (i.e. floats) we use DEFINE_L_SCALAR_OPERATION/DEFINE_R_SCALAR_OPERATION
		depending on whether the scalar is on the left or right side of the operation
		(i.e. 2 * a vs a * 2)
		And for ArrayReal scalars we use DEFINE_L_OPERATION/DEFINE_R_OPERATION

		As for division, we use specific scalar versions to increase performance (calculate
		the inverse of the scalar once, then multiply) as well as placing asserts in
		case of trying to divide by zero.

		I've considered using templates, but decided against it because wrong operator usage
		would raise cryptic compile error messages, and templates inability to limit which
		types are being used, leave the slight possibility of mixing completely unrelated
		types (i.e. ArrayQuaternion + ArrayVector4) and quietly compiling wrong code.
		Furthermore some platforms may not have decent compilers handling templates for
		major operator usage.

		Advantages:
			* Increased readability
			* Ease of reading & understanding
		Disadvantages:
			* Debugger can't go inside the operator. See workaround.

		As a workaround to the disadvantage, you can compile this code using cl.exe /EP /P /C to
		generate this file with macros replaced as actual code (very handy!)
	*/
	// Arithmetic operations
#define DEFINE_OPERATION( leftClass, rightClass, op, op_func )\
	inline ArrayVector3 operator op ( const leftClass &lhs, const rightClass &rhs )\
	{\
		const ArrayReal * RESTRICT_ALIAS lhsChunkBase = lhs.m_chunkBase;\
		const ArrayReal * RESTRICT_ALIAS rhsChunkBase = rhs.m_chunkBase;\
		return ArrayVector3(\
				op_func( lhsChunkBase[0], rhsChunkBase[0] ),\
				op_func( lhsChunkBase[1], rhsChunkBase[1] ),\
				op_func( lhsChunkBase[2], rhsChunkBase[2] ) );\
	   }
#define DEFINE_L_SCALAR_OPERATION( leftType, rightClass, op, op_func )\
	inline ArrayVector3 operator op ( const leftType fScalar, const rightClass &rhs )\
	{\
		ArrayReal lhs = _mm_set1_ps( fScalar );\
		return ArrayVector3(\
				op_func( lhs, rhs.m_chunkBase[0] ),\
				op_func( lhs, rhs.m_chunkBase[1] ),\
				op_func( lhs, rhs.m_chunkBase[2] ) );\
	}
#define DEFINE_R_SCALAR_OPERATION( leftClass, rightType, op, op_func )\
	inline ArrayVector3 operator op ( const leftClass &lhs, const rightType fScalar )\
	{\
		ArrayReal rhs = _mm_set1_ps( fScalar );\
		return ArrayVector3(\
				op_func( lhs.m_chunkBase[0], rhs ),\
				op_func( lhs.m_chunkBase[1], rhs ),\
				op_func( lhs.m_chunkBase[2], rhs ) );\
	}
#define DEFINE_L_OPERATION( leftType, rightClass, op, op_func )\
	inline ArrayVector3 operator op ( const leftType lhs, const rightClass &rhs )\
	{\
		return ArrayVector3(\
				op_func( lhs, rhs.m_chunkBase[0] ),\
				op_func( lhs, rhs.m_chunkBase[1] ),\
				op_func( lhs, rhs.m_chunkBase[2] ) );\
	}
#define DEFINE_R_OPERATION( leftClass, rightType, op, op_func )\
	inline ArrayVector3 operator op ( const leftClass &lhs, const rightType rhs )\
	{\
		return ArrayVector3(\
				op_func( lhs.m_chunkBase[0], rhs ),\
				op_func( lhs.m_chunkBase[1], rhs ),\
				op_func( lhs.m_chunkBase[2], rhs ) );\
	}

#define DEFINE_L_SCALAR_DIVISION( leftType, rightClass, op, op_func )\
	inline ArrayVector3 operator op ( const leftType fScalar, const rightClass &rhs )\
	{\
		ArrayReal lhs = _mm_set1_ps( fScalar );\
		return ArrayVector3(\
				op_func( lhs, rhs.m_chunkBase[0] ),\
				op_func( lhs, rhs.m_chunkBase[1] ),\
				op_func( lhs, rhs.m_chunkBase[2] ) );\
	}
#define DEFINE_R_SCALAR_DIVISION( leftClass, rightType, op, op_func )\
	inline ArrayVector3 operator op ( const leftClass &lhs, const rightType fScalar )\
	{\
		assert( fScalar != 0.0 );\
		Real fInv = 1.0f / fScalar;\
		ArrayReal rhs = _mm_set1_ps( fInv );\
		return ArrayVector3(\
				op_func( lhs.m_chunkBase[0], rhs ),\
				op_func( lhs.m_chunkBase[1], rhs ),\
				op_func( lhs.m_chunkBase[2], rhs ) );\
	}

#ifdef NDEBUG
	#define ASSERT_DIV_BY_ZERO( values ) ((void)0)
#else
	#define ASSERT_DIV_BY_ZERO( values ) {\
				assert( _mm_movemask_ps( _mm_cmpeq_ps( values, _mm_setzero_ps() ) ) == 0 &&\
				"One of the 4 floats is a zero. Can't divide by zero" ); }
#endif

#define DEFINE_L_DIVISION( leftType, rightClass, op, op_func )\
	inline ArrayVector3 operator op ( const leftType lhs, const rightClass &rhs )\
	{\
		return ArrayVector3(\
				op_func( lhs, rhs.m_chunkBase[0] ),\
				op_func( lhs, rhs.m_chunkBase[1] ),\
				op_func( lhs, rhs.m_chunkBase[2] ) );\
	}
#define DEFINE_R_DIVISION( leftClass, rightType, op, op_func )\
	inline ArrayVector3 operator op ( const leftClass &lhs, const rightType r )\
	{\
		ASSERT_DIV_BY_ZERO( r );\
		ArrayReal rhs = MathlibSSE2::Inv4( r );\
		return ArrayVector3(\
				op_func( lhs.m_chunkBase[0], rhs ),\
				op_func( lhs.m_chunkBase[1], rhs ),\
				op_func( lhs.m_chunkBase[2], rhs ) );\
	}

	// Update operations
#define DEFINE_UPDATE_OPERATION( leftClass, op, op_func )\
	inline void ArrayVector3::operator op ( const leftClass &a )\
	{\
		ArrayReal * RESTRICT_ALIAS chunkBase = m_chunkBase;\
		const ArrayReal * RESTRICT_ALIAS aChunkBase = a.m_chunkBase;\
		chunkBase[0] = op_func( chunkBase[0], aChunkBase[0] );\
		chunkBase[1] = op_func( chunkBase[1], aChunkBase[1] );\
		chunkBase[2] = op_func( chunkBase[2], aChunkBase[2] );\
	}
#define DEFINE_UPDATE_R_SCALAR_OPERATION( rightType, op, op_func )\
	inline void ArrayVector3::operator op ( const rightType fScalar )\
	{\
		ArrayReal a = _mm_set1_ps( fScalar );\
		m_chunkBase[0] = op_func( m_chunkBase[0], a );\
		m_chunkBase[1] = op_func( m_chunkBase[1], a );\
		m_chunkBase[2] = op_func( m_chunkBase[2], a );\
	}
#define DEFINE_UPDATE_R_OPERATION( rightType, op, op_func )\
	inline void ArrayVector3::operator op ( const rightType a )\
	{\
		m_chunkBase[0] = op_func( m_chunkBase[0], a );\
		m_chunkBase[1] = op_func( m_chunkBase[1], a );\
		m_chunkBase[2] = op_func( m_chunkBase[2], a );\
	}
#define DEFINE_UPDATE_DIVISION( leftClass, op, op_func )\
	inline void ArrayVector3::operator op ( const leftClass &a )\
	{\
		ArrayReal * RESTRICT_ALIAS chunkBase = m_chunkBase;\
		const ArrayReal * RESTRICT_ALIAS aChunkBase = a.m_chunkBase;\
		chunkBase[0] = op_func( chunkBase[0], aChunkBase[0] );\
		chunkBase[1] = op_func( chunkBase[1], aChunkBase[1] );\
		chunkBase[2] = op_func( chunkBase[2], aChunkBase[2] );\
	}
#define DEFINE_UPDATE_R_SCALAR_DIVISION( rightType, op, op_func )\
	inline void ArrayVector3::operator op ( const rightType fScalar )\
	{\
		assert( fScalar != 0.0 );\
		Real fInv = 1.0f / fScalar;\
		ArrayReal a = _mm_set1_ps( fInv );\
		m_chunkBase[0] = op_func( m_chunkBase[0], a );\
		m_chunkBase[1] = op_func( m_chunkBase[1], a );\
		m_chunkBase[2] = op_func( m_chunkBase[2], a );\
	}
#define DEFINE_UPDATE_R_DIVISION( rightType, op, op_func )\
	inline void ArrayVector3::operator op ( const rightType _a )\
	{\
		ASSERT_DIV_BY_ZERO( _a );\
		ArrayReal a = MathlibSSE2::Inv4( _a );\
		m_chunkBase[0] = op_func( m_chunkBase[0], a );\
		m_chunkBase[1] = op_func( m_chunkBase[1], a );\
		m_chunkBase[2] = op_func( m_chunkBase[2], a );\
	}

	inline const ArrayVector3& ArrayVector3::operator + () const
	{
		return *this;
	};
	//-----------------------------------------------------------------------------------
	inline ArrayVector3 ArrayVector3::operator - () const
	{
		return ArrayVector3(
			_mm_xor_ps( m_chunkBase[0], MathlibSSE2::SIGN_MASK ),	//-x
			_mm_xor_ps( m_chunkBase[1], MathlibSSE2::SIGN_MASK ),	//-y
			_mm_xor_ps( m_chunkBase[2], MathlibSSE2::SIGN_MASK ) );	//-z
	}
	//-----------------------------------------------------------------------------------

	// + Addition
	DEFINE_OPERATION( ArrayVector3, ArrayVector3, +, _mm_add_ps );
	DEFINE_L_SCALAR_OPERATION( Real, ArrayVector3, +, _mm_add_ps );
	DEFINE_R_SCALAR_OPERATION( ArrayVector3, Real, +, _mm_add_ps );

	DEFINE_L_OPERATION( ArrayReal, ArrayVector3, +, _mm_add_ps );
	DEFINE_R_OPERATION( ArrayVector3, ArrayReal, +, _mm_add_ps );

	// - Subtraction
	DEFINE_OPERATION( ArrayVector3, ArrayVector3, -, _mm_sub_ps );
	DEFINE_L_SCALAR_OPERATION( Real, ArrayVector3, -, _mm_sub_ps );
	DEFINE_R_SCALAR_OPERATION( ArrayVector3, Real, -, _mm_sub_ps );

	DEFINE_L_OPERATION( ArrayReal, ArrayVector3, -, _mm_sub_ps );
	DEFINE_R_OPERATION( ArrayVector3, ArrayReal, -, _mm_sub_ps );

	// * Multiplication
	DEFINE_OPERATION( ArrayVector3, ArrayVector3, *, _mm_mul_ps );
	DEFINE_L_SCALAR_OPERATION( Real, ArrayVector3, *, _mm_mul_ps );
	DEFINE_R_SCALAR_OPERATION( ArrayVector3, Real, *, _mm_mul_ps );

	DEFINE_L_OPERATION( ArrayReal, ArrayVector3, *, _mm_mul_ps );
	DEFINE_R_OPERATION( ArrayVector3, ArrayReal, *, _mm_mul_ps );

	// / Division (scalar versions use mul instead of div, because they mul against the reciprocal)
	DEFINE_OPERATION( ArrayVector3, ArrayVector3, /, _mm_div_ps );
	DEFINE_L_SCALAR_DIVISION( Real, ArrayVector3, /, _mm_div_ps );
	DEFINE_R_SCALAR_DIVISION( ArrayVector3, Real, /, _mm_mul_ps );

	DEFINE_L_DIVISION( ArrayReal, ArrayVector3, /, _mm_div_ps );
	DEFINE_R_DIVISION( ArrayVector3, ArrayReal, /, _mm_mul_ps );

	inline ArrayVector3 ArrayVector3::Cmov4( const ArrayVector3 &arg1, const ArrayVector3 &arg2, ArrayReal mask )
	{
		return ArrayVector3(
				MathlibSSE2::Cmov4( arg1.m_chunkBase[0], arg2.m_chunkBase[0], mask ),
				MathlibSSE2::Cmov4( arg1.m_chunkBase[1], arg2.m_chunkBase[1], mask ),
				MathlibSSE2::Cmov4( arg1.m_chunkBase[2], arg2.m_chunkBase[2], mask ) );
	}

	//-----------------------------------------------------------------------------------
	// START CODE TO BE COPIED TO OgreArrayVector3.inl
	//	Copy paste and replace "ArrayVector3::" for "ArrayVector3::"
	//-----------------------------------------------------------------------------------

	// Update operations
	// +=
	DEFINE_UPDATE_OPERATION(			ArrayVector3,		+=, _mm_add_ps );
	DEFINE_UPDATE_R_SCALAR_OPERATION(	Real,				+=, _mm_add_ps );
	DEFINE_UPDATE_R_OPERATION(			ArrayReal,			+=, _mm_add_ps );

	// -=
	DEFINE_UPDATE_OPERATION(			ArrayVector3,		-=, _mm_sub_ps );
	DEFINE_UPDATE_R_SCALAR_OPERATION(	Real,				-=, _mm_sub_ps );
	DEFINE_UPDATE_R_OPERATION(			ArrayReal,			-=, _mm_sub_ps );

	// *=
	DEFINE_UPDATE_OPERATION(			ArrayVector3,		*=, _mm_mul_ps );
	DEFINE_UPDATE_R_SCALAR_OPERATION(	Real,				*=, _mm_mul_ps );
	DEFINE_UPDATE_R_OPERATION(			ArrayReal,			*=, _mm_mul_ps );

	// /=
	DEFINE_UPDATE_DIVISION(				ArrayVector3,		/=, _mm_div_ps );
	DEFINE_UPDATE_R_SCALAR_DIVISION(	Real,				/=, _mm_mul_ps );
	DEFINE_UPDATE_R_DIVISION(			ArrayReal,			/=, _mm_mul_ps );

	//Functions
	//-----------------------------------------------------------------------------------
	inline ArrayReal ArrayVector3::length() const
    {
		return
		_mm_sqrt_ps( _mm_add_ps( _mm_add_ps(					//sqrt(
				_mm_mul_ps( m_chunkBase[0], m_chunkBase[0] ),	//(x * x +
				_mm_mul_ps( m_chunkBase[1], m_chunkBase[1] ) ),	//y * y) +
			_mm_mul_ps( m_chunkBase[2], m_chunkBase[2] ) ) );	//z * z )
    }
	//-----------------------------------------------------------------------------------
	inline ArrayReal ArrayVector3::squaredLength() const
    {
        return
		_mm_add_ps( _mm_add_ps(
			_mm_mul_ps( m_chunkBase[0], m_chunkBase[0] ),	//(x * x +
			_mm_mul_ps( m_chunkBase[1], m_chunkBase[1] ) ),	//y * y) +
		_mm_mul_ps( m_chunkBase[2], m_chunkBase[2] ) );		//z * z )
    }
	//-----------------------------------------------------------------------------------
	inline ArrayReal ArrayVector3::distance( const ArrayVector3& rhs ) const
    {
        return (*this - rhs).length();
    }
	//-----------------------------------------------------------------------------------
	inline ArrayReal ArrayVector3::squaredDistance( const ArrayVector3& rhs ) const
    {
        return (*this - rhs).squaredLength();
    }
	//-----------------------------------------------------------------------------------
	inline ArrayReal ArrayVector3::dotProduct( const ArrayVector3& vec ) const
	{
		return
		_mm_add_ps( _mm_add_ps(
			_mm_mul_ps( m_chunkBase[0], vec.m_chunkBase[0] ) ,	//( x * vec.x   +
			_mm_mul_ps( m_chunkBase[1], vec.m_chunkBase[1] ) ),	//  y * vec.y ) +
			_mm_mul_ps( m_chunkBase[2], vec.m_chunkBase[2] ) );	//  z * vec.z
	}
	//-----------------------------------------------------------------------------------
	inline ArrayReal ArrayVector3::absDotProduct( const ArrayVector3& vec ) const
	{
		return
		_mm_add_ps( _mm_add_ps(
			MathlibSSE2::Abs4( _mm_mul_ps( m_chunkBase[0], vec.m_chunkBase[0] ) ),	//( abs( x * vec.x )   +
			MathlibSSE2::Abs4( _mm_mul_ps( m_chunkBase[1], vec.m_chunkBase[1] ) ) ),//  abs( y * vec.y ) ) +
			MathlibSSE2::Abs4( _mm_mul_ps( m_chunkBase[2], vec.m_chunkBase[2] ) ) );//  abs( z * vec.z )
	}
	//-----------------------------------------------------------------------------------
	inline void ArrayVector3::normalise( void )
	{
		ArrayReal sqLength = _mm_add_ps( _mm_add_ps(
			_mm_mul_ps( m_chunkBase[0], m_chunkBase[0] ),	//(x * x +
			_mm_mul_ps( m_chunkBase[1], m_chunkBase[1] ) ),	//y * y) +
		_mm_mul_ps( m_chunkBase[2], m_chunkBase[2] ) );		//z * z )

		//Convert sqLength's 0s into 1, so that zero vectors remain as zero
		//Denormals are treated as 0 during the check.
		//Note: We could create a mask now and nuke nans after InvSqrt, however
		//generating the nans could impact performance in some architectures
		sqLength = MathlibSSE2::Cmov4( sqLength, MathlibSSE2::ONE,
										_mm_cmpgt_ps( sqLength, MathlibSSE2::FLOAT_MIN ) );
		ArrayReal invLength = MathlibSSE2::InvSqrtNonZero4( sqLength );
		m_chunkBase[0] = _mm_mul_ps( m_chunkBase[0], invLength ); //x * invLength
		m_chunkBase[1] = _mm_mul_ps( m_chunkBase[1], invLength ); //y * invLength
		m_chunkBase[2] = _mm_mul_ps( m_chunkBase[2], invLength ); //z * invLength
	}
	//-----------------------------------------------------------------------------------
	inline ArrayVector3 ArrayVector3::crossProduct( const ArrayVector3& rkVec ) const
    {
        return ArrayVector3(
			_mm_sub_ps(
				_mm_mul_ps( m_chunkBase[1], rkVec.m_chunkBase[2] ),
				_mm_mul_ps( m_chunkBase[2], rkVec.m_chunkBase[1] ) ),	//y * rkVec.z - z * rkVec.y
			_mm_sub_ps(
				_mm_mul_ps( m_chunkBase[2], rkVec.m_chunkBase[0] ),
				_mm_mul_ps( m_chunkBase[0], rkVec.m_chunkBase[2] ) ),	//z * rkVec.x - x * rkVec.z
			_mm_sub_ps(
				_mm_mul_ps( m_chunkBase[0], rkVec.m_chunkBase[1] ),
				_mm_mul_ps( m_chunkBase[1], rkVec.m_chunkBase[0] ) ) );	//x * rkVec.y - y * rkVec.x
    }
	//-----------------------------------------------------------------------------------
	inline ArrayVector3 ArrayVector3::midPoint( const ArrayVector3& rkVec ) const
    {
		return ArrayVector3(
			_mm_mul_ps( _mm_add_ps( m_chunkBase[0], rkVec.m_chunkBase[0] ), MathlibSSE2::HALF ),
			_mm_mul_ps( _mm_add_ps( m_chunkBase[1], rkVec.m_chunkBase[1] ), MathlibSSE2::HALF ),
			_mm_mul_ps( _mm_add_ps( m_chunkBase[2], rkVec.m_chunkBase[2] ), MathlibSSE2::HALF ) );
	}
	//-----------------------------------------------------------------------------------
	inline void ArrayVector3::makeFloor( const ArrayVector3& cmp )
    {
		ArrayReal * RESTRICT_ALIAS aChunkBase = m_chunkBase;
		const ArrayReal * RESTRICT_ALIAS bChunkBase = cmp.m_chunkBase;
		aChunkBase[0] = _mm_min_ps( aChunkBase[0], bChunkBase[0] );
		aChunkBase[1] = _mm_min_ps( aChunkBase[1], bChunkBase[1] );
		aChunkBase[2] = _mm_min_ps( aChunkBase[2], bChunkBase[2] );
	}
	//-----------------------------------------------------------------------------------
	inline void ArrayVector3::makeCeil( const ArrayVector3& cmp )
    {
		ArrayReal * RESTRICT_ALIAS aChunkBase = m_chunkBase;
		const ArrayReal * RESTRICT_ALIAS bChunkBase = cmp.m_chunkBase;
		aChunkBase[0] = _mm_max_ps( aChunkBase[0], bChunkBase[0] );
		aChunkBase[1] = _mm_max_ps( aChunkBase[1], bChunkBase[1] );
		aChunkBase[2] = _mm_max_ps( aChunkBase[2], bChunkBase[2] );
	}
	//-----------------------------------------------------------------------------------
	inline ArrayReal ArrayVector3::getMinComponent() const
	{
		return _mm_min_ps( m_chunkBase[0], _mm_min_ps( m_chunkBase[1], m_chunkBase[2] ) );
	}
	//-----------------------------------------------------------------------------------
	inline ArrayReal ArrayVector3::getMaxComponent() const
	{
		return _mm_max_ps( m_chunkBase[0], _mm_max_ps( m_chunkBase[1], m_chunkBase[2] ) );
	}
	//-----------------------------------------------------------------------------------
	inline void ArrayVector3::setToSign()
	{
		// x = 1.0f | (x & 0x80000000)
		ArrayReal signMask = _mm_set1_ps( -0.0f );
		m_chunkBase[0] = _mm_or_ps( MathlibSSE2::ONE, _mm_and_ps( signMask, m_chunkBase[0] ) );
		m_chunkBase[1] = _mm_or_ps( MathlibSSE2::ONE, _mm_and_ps( signMask, m_chunkBase[1] ) );
		m_chunkBase[2] = _mm_or_ps( MathlibSSE2::ONE, _mm_and_ps( signMask, m_chunkBase[2] ) );
	}
	//-----------------------------------------------------------------------------------
	inline ArrayVector3 ArrayVector3::perpendicular( void ) const
    {
        ArrayVector3 perp = this->crossProduct( ArrayVector3::UNIT_X );

		const ArrayReal mask = _mm_cmple_ps( perp.squaredLength(), MathlibSSE2::fSqEpsilon );
        // Check length
        if( _mm_movemask_ps( mask ) )
        {
            /* One or more of these vectors are the X axis multiplied by a scalar,
			   so we have to use another axis for those.
            */
            ArrayVector3 perp1 = this->crossProduct( ArrayVector3::UNIT_Y );
			perp.m_chunkBase[0] = MathlibSSE2::Cmov4( perp1.m_chunkBase[0], perp.m_chunkBase[0], mask );
			perp.m_chunkBase[1] = MathlibSSE2::Cmov4( perp1.m_chunkBase[1], perp.m_chunkBase[1], mask );
			perp.m_chunkBase[2] = MathlibSSE2::Cmov4( perp1.m_chunkBase[2], perp.m_chunkBase[2], mask );
        }
		perp.normalise();

        return perp;
	}
	//-----------------------------------------------------------------------------------
	inline ArrayVector3 ArrayVector3::normalisedCopy( void ) const
	{
		ArrayReal sqLength = _mm_add_ps( _mm_add_ps(
			_mm_mul_ps( m_chunkBase[0], m_chunkBase[0] ),	//(x * x +
			_mm_mul_ps( m_chunkBase[1], m_chunkBase[1] ) ),	//y * y) +
		_mm_mul_ps( m_chunkBase[2], m_chunkBase[2] ) );		//z * z )

		//Convert sqLength's 0s into 1, so that zero vectors remain as zero
		//Denormals are treated as 0 during the check.
		//Note: We could create a mask now and nuke nans after InvSqrt, however
		//generating the nans could impact performance in some architectures
		sqLength = MathlibSSE2::Cmov4( sqLength, MathlibSSE2::ONE,
										_mm_cmpgt_ps( sqLength, MathlibSSE2::FLOAT_MIN ) );
		ArrayReal invLength = MathlibSSE2::InvSqrtNonZero4( sqLength );

		return ArrayVector3(
			_mm_mul_ps( m_chunkBase[0], invLength ),	//x * invLength
			_mm_mul_ps( m_chunkBase[1], invLength ),	//y * invLength
			_mm_mul_ps( m_chunkBase[2], invLength ) );	//z * invLength
	}
	//-----------------------------------------------------------------------------------
	inline ArrayVector3 ArrayVector3::reflect( const ArrayVector3& normal ) const
	{
        const ArrayReal twoPointZero = _mm_set_ps1( 2.0f );
 		return ( *this - ( _mm_mul_ss( twoPointZero, this->dotProduct( normal ) ) * normal ) );
	}
	//-----------------------------------------------------------------------------------
	inline int ArrayVector3::isNaN( void ) const
	{
		ArrayReal mask = _mm_and_ps( _mm_and_ps( 
			_mm_cmpeq_ps( m_chunkBase[0], m_chunkBase[0] ),
			_mm_cmpeq_ps( m_chunkBase[1], m_chunkBase[1] ) ),
			_mm_cmpeq_ps( m_chunkBase[2], m_chunkBase[2] ) );

		return _mm_movemask_ps( mask ) ^ 0x0000000f;
	}
	//-----------------------------------------------------------------------------------
	inline ArrayVector3 ArrayVector3::primaryAxis( void ) const
	{
		// We could've used some operators, i.e.
		// xVec = MathlibSSE2::Cmov( ArrayVector3::UNIT_X, ArrayVector3::NEGATIVE_UNIT_X )
		// and so forth, which would've increased readability considerably. However,
		// MSVC's compiler ability to do constant propagation & remove dead code sucks,
		// which means it would try to cmov the Y & Z component even though we already
		// know it's always zero for both +x & -x. Therefore, we do it the manual
		// way. Doing this the "human readable way" results in massive amounts of wasted
		// instructions and stack memory abuse.
		// See Vector3::primaryAxis() to understand what's actually going on.
		ArrayReal absx = MathlibSSE2::Abs4( m_chunkBase[0] );
		ArrayReal absy = MathlibSSE2::Abs4( m_chunkBase[1] );
		ArrayReal absz = MathlibSSE2::Abs4( m_chunkBase[2] );

		//xVec = x > 0 ? Vector3::UNIT_X : Vector3::NEGATIVE_UNIT_X;
		ArrayReal sign = MathlibSSE2::Cmov4( _mm_set1_ps( 1.0f ), _mm_set1_ps( -1.0f ),
											_mm_cmpgt_ps( m_chunkBase[0], _mm_setzero_ps() ) );
		ArrayVector3 xVec( _mm_mul_ps( _mm_set1_ps( 1.0f ), sign ),
								_mm_setzero_ps(), _mm_setzero_ps() );

		//yVec = y > 0 ? Vector3::UNIT_Y : Vector3::NEGATIVE_UNIT_Y;
		sign = MathlibSSE2::Cmov4( _mm_set1_ps( 1.0f ), _mm_set1_ps( -1.0f ),
									_mm_cmpgt_ps( m_chunkBase[1], _mm_setzero_ps() ) );
		ArrayVector3 yVec( _mm_setzero_ps(), _mm_mul_ps( _mm_set1_ps( 1.0f ), sign ),
								_mm_setzero_ps() );

		//zVec = z > 0 ? Vector3::UNIT_Z : Vector3::NEGATIVE_UNIT_Z;
		sign = MathlibSSE2::Cmov4( _mm_set1_ps( 1.0f ), _mm_set1_ps( -1.0f ),
									_mm_cmpgt_ps( m_chunkBase[2], _mm_setzero_ps() ) );
		ArrayVector3 zVec( _mm_setzero_ps(), _mm_setzero_ps(),
								_mm_mul_ps( _mm_set1_ps( 1.0f ), sign ) );

		//xVec = absx > absz ? xVec : zVec
		ArrayReal mask = _mm_cmpgt_ps( absx, absz );
		xVec.m_chunkBase[0] = MathlibSSE2::Cmov4( xVec.m_chunkBase[0], zVec.m_chunkBase[0], mask );
		xVec.m_chunkBase[2] = MathlibSSE2::Cmov4( xVec.m_chunkBase[2], zVec.m_chunkBase[2], mask );

		//yVec = absy > absz ? yVec : zVec
		mask = _mm_cmpgt_ps( absy, absz );
		yVec.m_chunkBase[1] = MathlibSSE2::Cmov4( yVec.m_chunkBase[1], zVec.m_chunkBase[1], mask );
		yVec.m_chunkBase[2] = MathlibSSE2::Cmov4( yVec.m_chunkBase[2], zVec.m_chunkBase[2], mask );

		xVec.Cmov4( _mm_cmpgt_ps( absx, absy ), yVec );
		return xVec;
	}
	//-----------------------------------------------------------------------------------
	inline Vector3 ArrayVector3::collapseMin( void ) const
	{
		OGRE_ALIGNED_DECL( Real, vals[4], OGRE_SIMD_ALIGNMENT );
		ArrayReal aosVec0, aosVec1, aosVec2, aosVec3;

		//Transpose XXXX YYYY ZZZZ to XYZZ XYZZ XYZZ XYZZ
		ArrayReal tmp2, tmp0;
		tmp0   = _mm_shuffle_ps( m_chunkBase[0], m_chunkBase[1], 0x44 );
        tmp2   = _mm_shuffle_ps( m_chunkBase[0], m_chunkBase[1], 0xEE );

        aosVec0 = _mm_shuffle_ps( tmp0, m_chunkBase[2], 0x08 );
		aosVec1 = _mm_shuffle_ps( tmp0, m_chunkBase[2], 0x5D );
		aosVec2 = _mm_shuffle_ps( tmp2, m_chunkBase[2], 0xA8 );
		aosVec3 = _mm_shuffle_ps( tmp2, m_chunkBase[2], 0xFD );

		//Do the actual operation
		aosVec0 = _mm_min_ps( aosVec0, aosVec1 );
		aosVec2 = _mm_min_ps( aosVec2, aosVec3 );
		aosVec0 = _mm_min_ps( aosVec0, aosVec2 );

		_mm_store_ps( vals, aosVec0 );

		return Vector3( vals[0], vals[1], vals[2] );
	}
	//-----------------------------------------------------------------------------------
	inline Vector3 ArrayVector3::collapseMax( void ) const
	{
		OGRE_ALIGNED_DECL( Real, vals[4], OGRE_SIMD_ALIGNMENT );
		ArrayReal aosVec0, aosVec1, aosVec2, aosVec3;

		//Transpose XXXX YYYY ZZZZ to XYZZ XYZZ XYZZ XYZZ
		ArrayReal tmp2, tmp0;
		tmp0   = _mm_shuffle_ps( m_chunkBase[0], m_chunkBase[1], 0x44 );
        tmp2   = _mm_shuffle_ps( m_chunkBase[0], m_chunkBase[1], 0xEE );

        aosVec0 = _mm_shuffle_ps( tmp0, m_chunkBase[2], 0x08 );
		aosVec1 = _mm_shuffle_ps( tmp0, m_chunkBase[2], 0x5D );
		aosVec2 = _mm_shuffle_ps( tmp2, m_chunkBase[2], 0xA8 );
		aosVec3 = _mm_shuffle_ps( tmp2, m_chunkBase[2], 0xFD );

		//Do the actual operation
		aosVec0 = _mm_max_ps( aosVec0, aosVec1 );
		aosVec2 = _mm_max_ps( aosVec2, aosVec3 );
		aosVec0 = _mm_max_ps( aosVec0, aosVec2 );

		_mm_store_ps( vals, aosVec0 );

		return Vector3( vals[0], vals[1], vals[2] );
	}
	//-----------------------------------------------------------------------------------
	inline void ArrayVector3::Cmov4( ArrayReal mask, const ArrayVector3 &replacement )
	{
		ArrayReal * RESTRICT_ALIAS aChunkBase = m_chunkBase;
		const ArrayReal * RESTRICT_ALIAS bChunkBase = replacement.m_chunkBase;
		aChunkBase[0] = MathlibSSE2::Cmov4( aChunkBase[0], bChunkBase[0], mask );
		aChunkBase[1] = MathlibSSE2::Cmov4( aChunkBase[1], bChunkBase[1], mask );
		aChunkBase[2] = MathlibSSE2::Cmov4( aChunkBase[2], bChunkBase[2], mask );
	}
	//-----------------------------------------------------------------------------------
	inline void ArrayVector3::CmovRobust( ArrayReal mask, const ArrayVector3 &replacement )
	{
		ArrayReal * RESTRICT_ALIAS aChunkBase = m_chunkBase;
		const ArrayReal * RESTRICT_ALIAS bChunkBase = replacement.m_chunkBase;
		aChunkBase[0] = MathlibSSE2::CmovRobust( aChunkBase[0], bChunkBase[0], mask );
		aChunkBase[1] = MathlibSSE2::CmovRobust( aChunkBase[1], bChunkBase[1], mask );
		aChunkBase[2] = MathlibSSE2::CmovRobust( aChunkBase[2], bChunkBase[2], mask );
	}
	//-----------------------------------------------------------------------------------

#undef DEFINE_OPERATION
#undef DEFINE_R_SCALAR_OPERATION
#undef DEFINE_R_OPERATION
#undef DEFINE_DIVISION
#undef DEFINE_R_SCALAR_DIVISION
#undef DEFINE_R_DIVISION

#undef DEFINE_UPDATE_OPERATION
#undef DEFINE_UPDATE_R_SCALAR_OPERATION
#undef DEFINE_UPDATE_R_OPERATION
#undef DEFINE_UPDATE_DIVISION
#undef DEFINE_UPDATE_R_SCALAR_DIVISION
#undef DEFINE_UPDATE_R_DIVISION

	//-----------------------------------------------------------------------------------
	// END CODE TO BE COPIED TO OgreArrayVector3.inl
	//-----------------------------------------------------------------------------------
}
