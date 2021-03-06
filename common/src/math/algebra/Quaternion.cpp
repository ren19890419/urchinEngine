#include <cassert>
#include <cmath>
#include <stdexcept>

#include "math/algebra/Quaternion.h"
#include "math/algebra/MathValue.h"
#include "math/algorithm/MathAlgorithm.h"

namespace urchin
{

	template<class T> Quaternion<T>::Quaternion() :
		X(0), Y(0), Z(0), W(1)
	{

	}

	template<class T> Quaternion<T>::Quaternion(T Xu, T Yu, T Zu, T Wu) :
		X(Xu), Y(Yu), Z(Zu), W(Wu)
	{

	}

	template<class T> Quaternion<T>::Quaternion(const Matrix3<T> &matrix)
	{
		const T trace = matrix(0, 0) + matrix(1, 1) + matrix(2, 2);

		if(trace > 0.0)
		{
			const T s = 0.5f / sqrtf(trace + 1.0f);
			X = (matrix(2, 1) - matrix(1, 2)) * s;
			Y = (matrix(0, 2) - matrix(2, 0)) * s;
			Z = (matrix(1, 0) - matrix(0, 1)) * s;
			W = 0.25f / s;
		}else
		{
			if((matrix(0, 0) > matrix(1, 1)) && (matrix(0, 0) > matrix(2, 2)))
			{
				const T s = sqrtf(1.0 + matrix(0, 0) - matrix(1, 1) - matrix(2, 2)) * 2.0;
				X = 0.25 * s;
				Y = (matrix(0, 1) + matrix(1, 0)) / s;
				Z = (matrix(0, 2) + matrix(2, 0)) / s;
				W = (matrix(2, 1) - matrix(1, 2)) / s;
			}else if (matrix(1, 1) > matrix(2, 2))
			{
				const T s = sqrtf(1.0 - matrix(0, 0) + matrix(1, 1) - matrix(2, 2)) * 2.0;
				X = (matrix(0, 1) + matrix(1, 0)) / s;
				Y = 0.25 * s;
				Z = (matrix(1, 2) + matrix(2, 1)) / s;
				W = (matrix(0, 2) - matrix(2, 0)) / s;
			}else
			{
				const T s = sqrtf(1.0 - matrix(0, 0) - matrix(1, 1) + matrix(2, 2)) * 2.0;
				X = (matrix(0, 2) + matrix(2, 0)) / s;
				Y = (matrix(1, 2) + matrix(2, 1)) / s;
				Z = 0.25 * s;
				W = (matrix(1, 0) - matrix(0, 1)) / s;
			}
		}
	}

	template<class T> Quaternion<T>::Quaternion(const Vector3<T> &axis, T angle)
	{
		const T halfAngle = angle / 2.0;
		const T sin = std::sin(halfAngle);

		Vector3<T> normalizedAxis = axis.normalize();
		X = normalizedAxis.X * sin;
		Y = normalizedAxis.Y * sin;
		Z = normalizedAxis.Z * sin;
		W = std::cos(halfAngle);
	}

	template<class T> Quaternion<T>::Quaternion(const Vector3<T> &eulerAngles, RotationSequence rotationSequence)
	{
		Quaternion<T> finalQuaternion;

		switch(rotationSequence)
		{
			case RotationSequence::XYZ:
				finalQuaternion = Quaternion(Vector3<T>(0.0, 0.0, 1.0), eulerAngles[2]) *
						Quaternion(Vector3<T>(0.0, 1.0, 0.0), eulerAngles[1]) *
						Quaternion(Vector3<T>(1.0, 0.0, 0.0), eulerAngles[0]);
				break;
			case RotationSequence::XZY:
				finalQuaternion = Quaternion(Vector3<T>(0.0, 1.0, 0.0), eulerAngles[2]) *
						Quaternion(Vector3<T>(0.0, 0.0, 1.0), eulerAngles[1]) *
						Quaternion(Vector3<T>(1.0, 0.0, 0.0), eulerAngles[0]);
				break;
			case RotationSequence::YXZ:
				finalQuaternion = Quaternion(Vector3<T>(0.0, 0.0, 1.0), eulerAngles[2]) *
						Quaternion(Vector3<T>(1.0, 0.0, 0.0), eulerAngles[1]) *
						Quaternion(Vector3<T>(0.0, 1.0, 0.0), eulerAngles[0]);
				break;
			case RotationSequence::YZX:
				finalQuaternion = Quaternion(Vector3<T>(1.0, 0.0, 0.0), eulerAngles[2]) *
						Quaternion(Vector3<T>(0.0, 0.0, 1.0), eulerAngles[1]) *
						Quaternion(Vector3<T>(0.0, 1.0, 0.0), eulerAngles[0]);
				break;
			case RotationSequence::ZXY:
				finalQuaternion = Quaternion(Vector3<T>(0.0, 1.0, 0.0), eulerAngles[2]) *
						Quaternion(Vector3<T>(1.0, 0.0, 0.0), eulerAngles[1]) *
						Quaternion(Vector3<T>(0.0, 0.0, 1.0), eulerAngles[0]);
				break;
			case RotationSequence::ZYX:
				finalQuaternion = Quaternion(Vector3<T>(1.0, 0.0, 0.0), eulerAngles[2]) *
						Quaternion(Vector3<T>(0.0, 1.0, 0.0), eulerAngles[1]) *
						Quaternion(Vector3<T>(0.0, 0.0, 1.0), eulerAngles[0]);
				break;
			case RotationSequence::XYX:
				finalQuaternion = Quaternion(Vector3<T>(1.0, 0.0, 0.0), eulerAngles[2]) *
						Quaternion(Vector3<T>(0.0, 1.0, 0.0), eulerAngles[1]) *
						Quaternion(Vector3<T>(1.0, 0.0, 0.0), eulerAngles[0]);
				break;
			case RotationSequence::XZX:
				finalQuaternion = Quaternion(Vector3<T>(1.0, 0.0, 0.0), eulerAngles[2]) *
						Quaternion(Vector3<T>(0.0, 0.0, 1.0), eulerAngles[1]) *
						Quaternion(Vector3<T>(1.0, 0.0, 0.0), eulerAngles[0]);
				break;
			case RotationSequence::YXY:
				finalQuaternion = Quaternion(Vector3<T>(0.0, 1.0, 0.0), eulerAngles[2]) *
						Quaternion(Vector3<T>(1.0, 0.0, 0.0), eulerAngles[1]) *
						Quaternion(Vector3<T>(0.0, 1.0, 0.0), eulerAngles[0]);
				break;
			case RotationSequence::YZY:
				finalQuaternion = Quaternion(Vector3<T>(0.0, 1.0, 0.0), eulerAngles[2]) *
						Quaternion(Vector3<T>(0.0, 0.0, 1.0), eulerAngles[1]) *
						Quaternion(Vector3<T>(0.0, 1.0, 0.0), eulerAngles[0]);
				break;
			case RotationSequence::ZXZ:
				finalQuaternion = Quaternion(Vector3<T>(0.0, 0.0, 1.0), eulerAngles[2]) *
						Quaternion(Vector3<T>(1.0, 0.0, 0.0), eulerAngles[1]) *
						Quaternion(Vector3<T>(0.0, 0.0, 1.0), eulerAngles[0]);
				break;
			case RotationSequence::ZYZ:
				finalQuaternion = Quaternion(Vector3<T>(0.0, 0.0, 1.0), eulerAngles[2]) *
						Quaternion(Vector3<T>(0.0, 1.0, 0.0), eulerAngles[1]) *
						Quaternion(Vector3<T>(0.0, 0.0, 1.0), eulerAngles[0]);
				break;
			default:
				throw std::invalid_argument("Unknown quaternion rotation sequence: " + std::to_string(rotationSequence));
		}

		X = finalQuaternion.X;
		Y = finalQuaternion.Y;
		Z = finalQuaternion.Z;
		W = finalQuaternion.W;
	}

    template<class T> Quaternion<T>::Quaternion(const Vector3<T> &normalizedLookAt, const Vector3<T> &normalizedUp)
    {
        #ifndef NDEBUG
            assert(MathAlgorithm::isOne(normalizedLookAt.length(), 0.001));
		    assert(MathAlgorithm::isOne(normalizedUp.length(), 0.001));
        #endif

        Vector3<T> right = normalizedUp.crossProduct(normalizedLookAt);
        T det = right.X + normalizedUp.Y + normalizedLookAt.Z;

        if(det > 0.0)
		{
        	T sqrtValue = sqrtf(1.0 + det);
			T halfOverSqrt = 0.5 / sqrtValue;
			X = (normalizedUp.Z - normalizedLookAt.Y) * halfOverSqrt;
			Y = (normalizedLookAt.X - right.Z) * halfOverSqrt;
			Z = (right.Y - normalizedUp.X) * halfOverSqrt;
            W = sqrtValue * 0.5;
		}else if ((right.X >= normalizedUp.Y) && (right.X >= normalizedLookAt.Z))
        {
            T sqrtValue = sqrtf(((1.0 + right.X) - normalizedUp.Y) - normalizedLookAt.Z);
            T halfOverSqrt = 0.5 / sqrtValue;
            X = 0.5 * sqrtValue;
            Y = (right.Y + normalizedUp.X) * halfOverSqrt;
            Z = (right.Z + normalizedLookAt.X) * halfOverSqrt;
            W = (normalizedUp.Z - normalizedLookAt.Y) * halfOverSqrt;
        }else if (normalizedUp.Y > normalizedLookAt.Z)
        {
            T sqrtValue = sqrtf(((1.0 + normalizedUp.Y) - right.X) - normalizedLookAt.Z);
            T halfOverSqrt = 0.5 / sqrtValue;
            X = (normalizedUp.X+ right.Y) * halfOverSqrt;
            Y = 0.5 * sqrtValue;
            Z = (normalizedLookAt.Y + normalizedUp.Z) * halfOverSqrt;
            W = (normalizedLookAt.X - right.Z) * halfOverSqrt;
        }else
        {
            T sqrtValue = sqrtf(((1.0 + normalizedLookAt.Z) - right.X) - normalizedUp.Y);
            T halfOverSqrt = 0.5 / sqrtValue;
            X = (normalizedLookAt.X + right.Z) * halfOverSqrt;
            Y = (normalizedLookAt.Y + normalizedUp.Z) * halfOverSqrt;
            Z = 0.5 * sqrtValue;
            W = (right.Y - normalizedUp.X) * halfOverSqrt;
        }
    }

	template<class T> void Quaternion<T>::computeW()
	{
		const T t = 1.0f - (X*X) - (Y*Y) - (Z*Z);

		if (t<0.0f)
		{
			W = 0.0f;
		}else
		{
			W = -sqrt(t);
		}
	}

	template<class T> void Quaternion<T>::setIdentity()
	{
		X = Y = Z = 0.0f;
		W = 1.0f;
	}

	template<class T> Quaternion<T> Quaternion<T>::normalize() const
	{
		const T normValue = norm();
	
		//checks for bogus norm, to protect against divide by zero
		if(normValue>0.0f)
		{
			return Quaternion<T>(X/normValue, Y/normValue, Z/normValue, W/normValue);
		}

		return Quaternion(X, Y, Z, W);
	}

	/**
	 * Return conjugate quaternion. In case of normalized quaternion: conjugate quaternion is equals to inverse quaternion. It's a faster alternative.
	 */
	template<class T> Quaternion<T> Quaternion<T>::conjugate() const
	{
		return Quaternion<T>(-X, -Y, -Z, W);
	}

	/**
	 * Return inverse quaternion. In case of normalized quaternion: use conjugate method to get same result with better performance.
	 */
	template<class T> Quaternion<T> Quaternion<T>::inverse() const
	{
		T squaredNorm = squareNorm();
		return Quaternion<T>(-X/squaredNorm, -Y/squaredNorm, -Z/squaredNorm, W/squaredNorm);
	}

	template<class T> T Quaternion<T>::norm() const
	{
		return std::sqrt((X*X) + (Y*Y) + (Z*Z) + (W*W));
	}

	template<class T> T Quaternion<T>::squareNorm() const
	{
		return (X*X) + (Y*Y) + (Z*Z) + (W*W);
	}

	template<class T> T Quaternion<T>::dotProduct(const Quaternion<T> &q) const
	{
		return ((X*q.X) + (Y*q.Y) + (Z*q.Z) + (W*q.W));
	}

	template<class T> Point3<T> Quaternion<T>::rotatePoint(const Point3<T> &point) const
	{
		//Rotate point only works with normalized quaternion
        #ifndef NDEBUG
			const T normValue = norm();
			assert(normValue >= (T)0.999);
			assert(normValue <= (T)1.001);
		#endif

		Quaternion<T> final = (*this) * point * this->conjugate();
		return Point3<T>(final.X, final.Y, final.Z);
	}

	template<class T> Quaternion<T> Quaternion<T>::slerp(const Quaternion &q, T t) const
	{
		//check for out-of range parameter and return edge points if so
		if(t <= 0.0)
		{
			return *this;
		}
		if(t >= 1.0)
		{
			return q;
		}

		T cosOmega = dotProduct(q);
		T qX = q.X;
		T qY = q.Y;
		T qZ = q.Z;
        T qW = q.W;

		if(cosOmega < 0.0)
		{ //ensure to take shortest path
            cosOmega = -cosOmega;
			qX = -qX;
			qY = -qY;
			qZ = -qZ;
            qW = -qW;
		}

        //we should have two unit quaternions, so dot should be <= 1.0
		assert(cosOmega < 1.1);

		//computes interpolation fraction, checking for quaternions almost exactly the same
		T k0, k1;

		if(cosOmega > 0.9999)
		{
			//very close - just use linear interpolation, which will protect against a divide by zero
			k0 = 1.0 - t;
			k1 = t;
		}else
		{
			//computes the sin of the angle using the trig identity sin^2(omega) + cos^2(omega) = 1
			T sinOmega = std::sqrt(1.0 - (cosOmega * cosOmega));

			//computes the angle from its sin and cosine
			T omega = std::atan2(sinOmega, cosOmega);

			//computes inverse of denominator, so we only have to divide once
			T oneOverSinOmega = 1.0 / sinOmega;

			//computes interpolation parameters
			k0 = std::sin((1.0 - t) * omega) * oneOverSinOmega;
			k1 = std::sin(t * omega) * oneOverSinOmega;
		}

		return Quaternion<T>(
				(k0 * X) + (k1 * qX),
				(k0 * Y) + (k1 * qY),
				(k0 * Z) + (k1 * qZ),
				(k0 * W) + (k1 * qW));
	}

	template<class T> Quaternion<T> Quaternion<T>::lerp(const Quaternion &q, T t) const
	{
        //check for out-of range parameter and return edge points if so
        if(t <= 0.0)
        {
            return *this;
        }
        if(t >= 1.0)
        {
            return q;
        }

        T cosOmega = dotProduct(q);
        T qX = q.X;
        T qY = q.Y;
        T qZ = q.Z;
        T qW = q.W;

		if(cosOmega < 0.0)
		{ //ensure to take shortest path
            qX = -qX;
            qY = -qY;
            qZ = -qZ;
            qW = -qW;
		}

		float oneMinusT = 1.0 - t;
		Quaternion<T> lerpResult(
				oneMinusT*X + t*qX,
				oneMinusT*Y + t*qY,
				oneMinusT*Z + t*qZ,
				oneMinusT*W + t*qW);
		return lerpResult.normalize();
	}

	template<class T> Vector3<T> Quaternion<T>::getForwardDirection() const
	{
		return rotatePoint(Point3<T>(0.0, 0.0, 1.0)).toVector();
	}

	template<class T> Matrix4<T> Quaternion<T>::toMatrix4() const
	{
		const T xx = X * X;
		const T xy = X * Y;
		const T xz = X * Z;
		const T xw = X * W;
		const T yy = Y * Y;
		const T yz = Y * Z;
		const T yw = Y * W;
		const T zz = Z * Z;
		const T zw = Z * W;

		return Matrix4<T>(	1 - 2 * (yy + zz),     	2 * (xy - zw),     	2 * (xz + yw), 		0,
				2 * (xy + zw), 		1 - 2 * (xx + zz),     	2 * (yz - xw), 		0,
				2 * (xz - yw),     	2 * (yz + xw), 		1 - 2 * (xx + yy), 	0,
				0,                 	0,                 	0, 			1);
	}

	template<class T> Matrix3<T> Quaternion<T>::toMatrix3() const
	{
		const T xx = X * X;
		const T xy = X * Y;
		const T xz = X * Z;
		const T xw = X * W;
		const T yy = Y * Y;
		const T yz = Y * Z;
		const T yw = Y * W;
		const T zz = Z * Z;
		const T zw = Z * W;

		return Matrix3<T>(	1 - 2 * (yy + zz),     	2 * (xy - zw),     	2 * (xz + yw),
				2 * (xy + zw), 		1 - 2 * (xx + zz),     	2 * (yz - xw),
				2 * (xz - yw),     	2 * (yz + xw), 		1 - 2 * (xx + yy));
	}

	template<class T> void Quaternion<T>::toAxisAngle(Vector3<T> &axis, T &angle) const
	{
		angle = std::acos(W) * 2.0;

		T norm = sqrtf(X*X + Y*Y + Z*Z);
		if(std::fabs(norm) > 0.0)
		{
			axis.X = X / norm;
			axis.Y = Y / norm;
			axis.Z = Z / norm;
		}else
		{
			axis.X = 0.0;
			axis.Y = 1.0;
			axis.Z = 0.0;
		}
	}

	template<class T> Vector3<T> Quaternion<T>::toEulerAngle(RotationSequence rotationSequence) const
	{
		switch(rotationSequence)
		{
			case RotationSequence::XYZ:
				return threeAxisEulerRotation(0, 1, 2);
			case RotationSequence::XZY:
				return threeAxisEulerRotation(0, 2, 1);
			case RotationSequence::YXZ:
				return threeAxisEulerRotation(1, 0, 2);
			case RotationSequence::YZX:
				return threeAxisEulerRotation(1, 2, 0);
			case RotationSequence::ZXY:
				return threeAxisEulerRotation(2, 0, 1);
			case RotationSequence::ZYX:
				return threeAxisEulerRotation(2, 1, 0);
			case RotationSequence::XYX:
				return twoAxisEulerRotation(0, 1, 2);
			case RotationSequence::XZX:
				return twoAxisEulerRotation(0, 2, 1);
			case RotationSequence::YXY:
				return twoAxisEulerRotation(1, 0, 2);
			case RotationSequence::YZY:
				return twoAxisEulerRotation(1, 2, 0);
			case RotationSequence::ZXZ:
				return twoAxisEulerRotation(2, 0, 1);
			case RotationSequence::ZYZ:
				return twoAxisEulerRotation(2, 1, 0);
			default:
				throw std::invalid_argument("Unknown quaternion rotation sequence: " + std::to_string(rotationSequence));
		}
	}

	template<class T> Vector3<T> Quaternion<T>::threeAxisEulerRotation(int i, int j, int k) const
	{ //inspired on Eigen library (EulerAngle.h)
		Vector3<T> euler;

		bool sequenceAxis = (i+1)%3==j;
		Matrix3<T> m = toMatrix3();

		euler[0] = std::atan2(m(k, j), m(k, k));
		T cosEuler1 = Vector2<T>(m(i, i), m(j, i)).length();

		if((sequenceAxis && euler[0]<0) || ((!sequenceAxis) && euler[0]>0))
		{
			euler[0] = (euler[0] > 0) ? euler[0] - PI_VALUE : euler[0] + PI_VALUE;
			euler[1] = std::atan2(-m(k, i), -cosEuler1);
		}else
		{
			euler[1] = std::atan2(-m(k, i), cosEuler1);
		}

		T sinEuler0 = sin(euler[0]);
		T cosEuler0 = cos(euler[0]);
		euler[2] = std::atan2(sinEuler0*m(i, k) - cosEuler0*m(i, j), cosEuler0*m(j, j) - sinEuler0*m(j, k));

		return sequenceAxis ? euler : -euler;
	}

	template<class T> Vector3<T> Quaternion<T>::twoAxisEulerRotation(int i, int j, int k) const
	{ //inspired on Eigen library (EulerAngle.h)
		Vector3<T> euler;

		bool sequenceAxis = (i+1)%3==j;
		Matrix3<T> m = toMatrix3();

		euler[0] = std::atan2(m(i, j), m(i, k));
		T sinEuler1 = Vector2<T>(m(i, j), m(i, k)).length();

	    if((sequenceAxis && euler[0]<0) || ((!sequenceAxis) && euler[0]>0))
	    {
	    	euler[0] = (euler[0] > 0) ? euler[0] - PI_VALUE : euler[0] + PI_VALUE;
	    	euler[1] = -std::atan2(sinEuler1, m(i, i));
	    }else
	    {
	    	euler[1] = std::atan2(sinEuler1, m(i, i));
	    }

	    T sinEuler0 = sin(euler[0]);
	    T cosEuler0 = cos(euler[0]);
	    euler[2] = std::atan2(cosEuler0*m(k, j) - sinEuler0*m(k, k), cosEuler0*m(j, j) - sinEuler0*m(j, k));

		return sequenceAxis ? euler : -euler;
	}

	template<class T> Quaternion<T> Quaternion<T>::operator +(const Quaternion<T> &q) const
	{
		return Quaternion<T>(X+q.X, Y+q.Y, Z+q.Z, W+q.W);
	}

	template<class T> Quaternion<T> Quaternion<T>::operator -(const Quaternion<T> &q) const
	{
		return Quaternion<T>(X-q.X, Y-q.Y, Z-q.Z, W-q.W);
	}

	template<class T> Quaternion<T> Quaternion<T>::operator *(const Quaternion<T> &q) const
	{
		return Quaternion<T>(
				W*q.X + X*q.W + Y*q.Z - Z*q.Y,
				W*q.Y - X*q.Z + Y*q.W + Z*q.X,
				W*q.Z + X*q.Y - Y*q.X + Z*q.W,
				W*q.W - X*q.X - Y*q.Y - Z*q.Z);
	}

	template<class T> const Quaternion<T>& Quaternion<T>::operator -=(const Quaternion<T> &q)
	{
		*this = *this - q;

		return *this;
	}

	template<class T> const Quaternion<T>& Quaternion<T>::operator +=(const Quaternion<T> &q)
	{
		*this = *this + q;

		return *this;
	}

	template<class T> const Quaternion<T>& Quaternion<T>::operator *=(const Quaternion<T> &q)
	{
		*this = *this * q;

		return *this;
	}

	template<class T> const Quaternion<T>& Quaternion<T>::operator *=(const Point3<T> &p)
	{
		*this = *this * p;

		return *this;
	}

	template<class T> const Quaternion<T>& Quaternion<T>::operator *=(T t)
	{
		*this = *this * t;

		return *this;
	}

	template<class T> bool Quaternion<T>::operator ==(const Quaternion<T> &q) const
	{
		return (X==q.X && Y==q.Y && Z==q.Z && W==q.W);
	}

	template<class T> bool Quaternion<T>::operator !=(const Quaternion<T> &q) const
	{
		return !(*this == q);
	}

	template<class T> T& Quaternion<T>::operator [](int i)
	{
		return (&X)[i];
	}

	template<class T> const T& Quaternion<T>::operator [](int i) const
	{
		return (&X)[i];
	}

	template<class T> Quaternion<T> operator *(const Quaternion<T> &q, const Point3<T> &p)
	{
		return Quaternion<T>(
				(q.W*p.X) + (q.Y*p.Z) - (q.Z*p.Y),
				(q.W*p.Y) + (q.Z*p.X) - (q.X*p.Z),
				(q.W*p.Z) + (q.X*p.Y) - (q.Y*p.X),
				-(q.X*p.X) - (q.Y*p.Y) - (q.Z*p.Z));
	}

	template<class T> Quaternion<T> operator *(const Quaternion<T> &q, T t)
	{
		return Quaternion<T>(q.X*t, q.Y*t, q.Z*t, q.W*t);
	}

	template<class T> Quaternion<T> operator *(T t, const Quaternion<T> &q)
	{
		return q * t;
	}

	template<class T> std::ostream& operator <<(std::ostream &stream, const Quaternion<T> &q)
	{
		return stream << q.X << " " << q.Y << " " << q.Z << " " << q.W;
	}

	//explicit template
	template class Quaternion<float>;
	template Quaternion<float> operator *(const Quaternion<float> &, const Point3<float> &);
	template Quaternion<float> operator *(const Quaternion<float> &, float);
	template Quaternion<float> operator *(float, const Quaternion<float> &);
	template std::ostream& operator <<<float>(std::ostream &, const Quaternion<float> &);

	template class Quaternion<double>;
	template Quaternion<double> operator *(const Quaternion<double> &, const Point3<double> &);
	template Quaternion<double> operator *(const Quaternion<double> &, double);
	template Quaternion<double> operator *(double, const Quaternion<double> &);
	template std::ostream& operator <<<double>(std::ostream &, const Quaternion<double> &);

}
