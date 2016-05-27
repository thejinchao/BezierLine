#include "bl_line.h"
#include <assert.h>
#include <float.h>
#include <complex>
//--------------------------------------------------------------------------------------
Bline::Bline()
	: m_keyPoints(nullptr)
	, m_keyCounts(0)
	, m_parts(nullptr)
	, m_partCounts(0)
{

}

//--------------------------------------------------------------------------------------
Bline::~Bline()
{
	release();
}

//--------------------------------------------------------------------------------------
void Bline::release(void)
{
	if (m_keyPoints) {
		delete[] m_keyPoints;
		m_keyPoints = 0;
	}

	m_keyCounts = 0;

	if (sizeof(Real) == sizeof(float)) {
		m_bounderMin.x = m_bounderMin.y = m_bounderMin.z = (Real)FLT_MAX;
		m_bounderMax.x = m_bounderMax.y = m_bounderMax.z = (Real)FLT_MIN;
	}
	else if(sizeof(Real) == sizeof(double)) {
		m_bounderMin.x = m_bounderMin.y = m_bounderMin.z = (Real)DBL_MAX;
		m_bounderMax.x = m_bounderMax.y = m_bounderMax.z = (Real)DBL_MIN;
	}

	if (m_parts) {
		delete[] m_parts;
		m_parts = 0;
	}
	m_partCounts = 0;
}

//--------------------------------------------------------------------------------------
void Bline::_middle(const Point& pt1, const Point& pt2, Point& middle)
{
	middle.x = (pt1.x + pt2.x) / 2.f;
	middle.y = (pt1.y + pt2.y) / 2.f;
	middle.z = (pt1.z + pt2.z) / 2.f;
}

//--------------------------------------------------------------------------------------
void Bline::_normalize(Point& vector)
{
	Real length = vector.x*vector.x + vector.y*vector.y + vector.z*vector.z;
	if (length < (Real)0.0000001) return;

	length = sqrt(length);
	vector.x /= length;
	vector.y /= length;
	vector.z /= length;
}

//--------------------------------------------------------------------------------------
Bline::Real Bline::_getlength(const LinePart& lp, Real t)
{
	const Real& A = lp.A;
	const Real& B = lp.B;
	const Real& C = lp.C;

	Real temp1 = sqrt(C + t*(B + A*t));
	Real temp2 = (2 * A*t*temp1 + B*(temp1 - lp.sqrt_C));
	Real temp4 = log(B + 2 * A*t + 2 * lp.sqrt_A*temp1);
	Real temp5 = 2 * lp.sqrt_A*temp2;
	Real temp6 = lp.E*(lp.D - temp4);

	return (temp5 + temp6) / (8 * pow(A, (Real)1.5));
}

//--------------------------------------------------------------------------------------
bool Bline::build(const Real* keyPoints, unsigned int keyCounts)
{
	release();

	assert(keyCounts >=3);

	m_keyCounts = keyCounts;
	m_keyPoints = new Point[keyCounts];

	const Real* k = keyPoints;
	for (unsigned int i = 0; i < keyCounts; i++) {
		Real& x = m_keyPoints[i].x = *k++;
		Real& y = m_keyPoints[i].y = *k++;
		Real& z = m_keyPoints[i].z = *k++;

		if (x > m_bounderMax.x) m_bounderMax.x = x;
		if (x < m_bounderMin.x) m_bounderMin.x = x;
		if (y > m_bounderMax.y) m_bounderMax.y = y;
		if (y < m_bounderMin.y) m_bounderMin.y = y;
		if (z > m_bounderMax.z) m_bounderMax.z = z;
		if (z < m_bounderMin.z) m_bounderMin.z = z;
	}

	m_totalLength = (Real)0.0;
	m_partCounts = keyCounts - 2;
	m_parts = new LinePart[m_partCounts];
	for (size_t i = 0; i < m_partCounts; i++) {
		LinePart& lp = m_parts[i];

		if (i == 0) {
			lp.pt0 = m_keyPoints[i];
		}
		else {
			_middle(m_keyPoints[i], m_keyPoints[i + 1], lp.pt0);
		}

		lp.pt1 = m_keyPoints[i + 1];

		if (i == m_partCounts - 1) {
			lp.pt2 = m_keyPoints[i + 2];
		}
		else {
			_middle(m_keyPoints[i + 1], m_keyPoints[i + 2], lp.pt2);
		}

		Real ax = lp.pt0.x - 2 * lp.pt1.x + lp.pt2.x;
		Real ay = lp.pt0.y - 2 * lp.pt1.y + lp.pt2.y;
		Real bx = 2 * lp.pt1.x - 2 * lp.pt0.x;
		Real by = 2 * lp.pt1.y - 2 * lp.pt0.y;

		lp.A = 4 * (ax*ax + ay*ay);
		lp.B = 4 * (ax*bx + ay*by);
		lp.C = bx*bx + by*by;
		lp.sqrt_A = sqrt(lp.A);
		lp.sqrt_C = sqrt(lp.C);
		lp.D = log(lp.B + 2 * lp.sqrt_A*lp.sqrt_C);
		lp.E = (lp.B*lp.B - 4*lp.A*lp.C);

		lp.length = _getlength(lp, (Real)1.0);

		m_totalLength += lp.length;
	}

	Real lengthAddup = (Real)0.0;
	for (size_t i = 0; i < m_partCounts; i++) {
		LinePart& lp = m_parts[i];

		lengthAddup += lp.length;

		lp.percent = lp.length / m_totalLength;
		lp.percentAddup = lengthAddup / m_totalLength;
	}
	return true;
}

//--------------------------------------------------------------------------------------
void Bline::getBounder(Point& min, Point& max) const
{
	min = m_bounderMin;
	max = m_bounderMax;
}

//--------------------------------------------------------------------------------------
Bline::Real Bline::_getInvertLength(const LinePart& lp, Real t, Real length) 
{
	Bline::Real t1 = t, t2;

	do {
		t2 = t1 - (_getlength(lp, t1) - length) / sqrt(lp.A*t1*t1 + lp.B*t1 + lp.C);
		if (fabs((t1 - t2))<0.000001) break;
		t1 = t2;
	} while (true);
	return t2;
}

//--------------------------------------------------------------------------------------
void Bline::getPoint(Real t, Point& point, Point& tangent) const
{
	assert(t >= (Real)0.0 && t <= (Real)1.0);

	if (t <= (Real)0.0) {
		point = m_keyPoints[0];
		tangent.x = -2 * point.x + 2 * m_keyPoints[1].x;
		tangent.y = -2 * point.y + 2 * m_keyPoints[1].y;
		tangent.z = -2 * point.z + 2 * m_keyPoints[1].z;
		_normalize(tangent);
		return;
	}

	size_t partIndex = m_partCounts;
	for (size_t i = 0; i < m_partCounts; i++) {
		if (m_parts[i].percentAddup >= t) {
			partIndex = i;
			break;
		}
	}

	if (partIndex >= m_partCounts) {
		point = m_keyPoints[m_keyCounts - 1];
		tangent.x = -2 * m_keyPoints[m_keyCounts - 2].x + 2 * point.x;
		tangent.y = -2 * m_keyPoints[m_keyCounts - 2].y + 2 * point.y;
		tangent.z = -2 * m_keyPoints[m_keyCounts - 2].z + 2 * point.z;
		_normalize(tangent);
		return;
	}

	const LinePart& lp = m_parts[partIndex];
	Real start_percent = (partIndex == 0) ? (Real)0.0 : m_parts[partIndex - 1].percentAddup;

	Real percent = (t - start_percent) / lp.percent;
	Real length = percent*lp.length;
	t = _getInvertLength(lp, percent, length);

	point.x = (1 - t)*(1 - t)*lp.pt0.x + 2 * (1 - t)*t*lp.pt1.x + t*t*lp.pt2.x;
	point.y = (1 - t)*(1 - t)*lp.pt0.y + 2 * (1 - t)*t*lp.pt1.y + t*t*lp.pt2.y;
	point.z = (1 - t)*(1 - t)*lp.pt0.z + 2 * (1 - t)*t*lp.pt1.z + t*t*lp.pt2.z;

	tangent.x = 2 * (t - 1)*lp.pt0.x + (2 - 4 * t)*lp.pt1.x + 2 * t*lp.pt2.x;
	tangent.y = 2 * (t - 1)*lp.pt0.y + (2 - 4 * t)*lp.pt1.y + 2 * t*lp.pt2.y;
	tangent.z = 2 * (t - 1)*lp.pt0.z + (2 - 4 * t)*lp.pt1.z + 2 * t*lp.pt2.z;
	_normalize(tangent);

	return;
}