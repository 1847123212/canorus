/*
 * Copyright (c) 2007, Matevž Jekovec, Canorus development team
 * All Rights Reserved. See AUTHORS for a complete list of authors.
 *
 * Licensed under the GNU GENERAL PUBLIC LICENSE. See LICENSE.GPL for details.
 */

#ifndef LILYPONDIMPORT_H_
#define LILYPONDIMPORT_H_

#include <QString>
#include <QStack>

#include "core/voice.h"
#include "core/rest.h"
#include "core/keysignature.h"
#include "core/clef.h"
#include "core/timesignature.h"
#include "core/barline.h"

class CALilyPondImport {
public:
	// Import voice
	CALilyPondImport(QString& in, CAVoice *voice);
	virtual ~CALilyPondImport();
	
	int curLine() { return _curLine; }
	int curChar() { return _curChar; }
	
private:
	static const QRegExp WHITESPACE_DELIMITERS;
	static const QRegExp SYNTAX_DELIMITERS;
	static const QRegExp DELIMITERS;
	
	// Internal pitch structure
	struct CAPitch {
		int pitch;
		signed char accs;
	};
	
	// Internal length
	struct CALength {
		CAPlayable::CAPlayableLength length;
		int dotted;
	};
	
	// Internal time signature
	struct CATime {
		int beats;
		int beat;
	};
	
	enum CALilyPondDepth {
		Score,
		Layout,
		Voice,
		Chord
	};
	
	// Do the actual import on the current voice and input string
	bool importVoice(CAVoice *voice);
	
	inline CAVoice *curVoice() { return _curVoice; }
	inline void setCurVoice(CAVoice *voice) { _curVoice = voice; }
	
	const QString parseNextElement();
	const QString peekNextElement();
	void addError(QString description, int lineError = 0, int charError = 0);
	
	//////////////////////
	// Helper functions //
	//////////////////////
	CALength playableLengthFromLilyPond(const QString playableElt);
	
	bool isNote(const QString elt);
	CAPitch relativePitchFromLilyPond(const QString note, int prevPitch);
	bool isRest(const QString elt);
	CARest::CARestType restTypeFromLilyPond(const QString rest);
	CAClef::CAClefType clefTypeFromLilyPond(const QString clef);
	signed char keySigAccsFromLilyPond(QString keySig, CAKeySignature::CAMajorMinorGender gender);
	CAKeySignature::CAMajorMinorGender keySigGenderFromLilyPond(QString gender);
	CATime timeSigFromLilyPond(QString time);
		
	CAMusElement* findSharedElement(CAMusElement *elt);
	
	///////////////////////////
	// Getter/Setter methods //
	///////////////////////////
	inline QString& in() { return _in; }
	inline CALilyPondDepth curDepth() { return _depth.top(); }
	inline void pushDepth(CALilyPondDepth depth) { _depth.push(depth); }
	inline CALilyPondDepth popDepth() { return _depth.pop(); }
	
	// Attributes
	QString _in;
	CAVoice *_curVoice;
	QStack<CALilyPondDepth> _depth;
	int _curLine, _curChar;
	QList<QString> _errors;
	QList<QString> _warnings;
};

#endif /*LILYPONDIMPORT_H_*/