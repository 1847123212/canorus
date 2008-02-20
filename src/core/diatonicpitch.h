/*!
	Copyright (c) 2008, Matevž Jekovec, Canorus development team
	All Rights Reserved. See AUTHORS for a complete list of authors.
	
	Licensed under the GNU GENERAL PUBLIC LICENSE. See LICENSE.GPL for details.
*/

#ifndef DIATONICPITCH_H_
#define DIATONICPITCH_H_

#include "core/interval.h"
#include <QString>

class CADiatonicPitch {
public:
	enum CANoteName {
		Undefined = -1,
		C = 0,
		D = 1,
		E = 2,
		F = 3,
		G = 4,
		A = 5,
		B = 6
	};
	
	CADiatonicPitch();
	CADiatonicPitch( const QString& pitch );
	CADiatonicPitch( const int& noteName, const int& accs=0 );
	
	bool operator==(CADiatonicPitch);
	inline bool operator!=(CADiatonicPitch p) { return !operator==(p); }
	
	bool operator==(int noteName);
	inline bool operator!=(int p) { return !operator==(p); }
	
	CADiatonicPitch operator+(CAInterval);
	CADiatonicPitch operator-(CAInterval i) {
		return operator+( CAInterval( i.quality(), i.quantity()*(-1) ) );
	}
	
	inline const int noteName() { return _noteName; }
	inline const int accs() { return _accs; }
	
	inline void setNoteName( const int noteName ) { _noteName = noteName; }
	inline void setAccs( const int accs ) { _accs = accs; }
	
	static const QString diatonicPitchToString( CADiatonicPitch p );
	
private:
	int _noteName; // 0-sub-contra C, 1-D, 2-E etc.
	int _accs;     // 0-neutral, 1-sharp, -1-flat etc.
};
#endif /* DIATONICPITCH_H_ */