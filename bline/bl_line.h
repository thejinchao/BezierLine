#pragma once

class Bline
{
public:
	typedef double Real;

	struct Point
	{
		Real x, y, z;
	};

	void release(void);
	bool build(const Real* keyPoints, unsigned int keyCounts);

	void	getBounder(Point& min, Point& max) const;
	size_t	getKeyCounts(void) const { return m_keyCounts; }
	Point*	getKeys(void) const { return m_keyPoints; }
	void	getPoint(Real t, Point& point, Point& tangent) const;

private:
	Point*		m_keyPoints;
	size_t		m_keyCounts;
	Point		m_bounderMin;
	Point		m_bounderMax;

	struct LinePart
	{
		Point pt0;
		Point pt1;
		Point pt2;
		Real length;

		//Cache
		Real A, B, C;
		Real sqrt_A, sqrt_C, D, E;

		//proportion
		Real percent;
		Real percentAddup;
	};

	LinePart*	m_parts;
	size_t		m_partCounts;
	Real		m_totalLength;

private:
	static void _middle(const Point& pt1, const Point& pt2, Point& middle);
	static void _normalize(Point& vector);
	static Real _getlength(const LinePart& lp, Real t);
	static Real _getInvertLength(const LinePart& lp, Real t, Real length);

public:
	Bline();
	virtual ~Bline();
};