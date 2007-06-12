// copyright 2003 stefan kersten <steve@k-hornz.de>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA

EmacsDocument : Document
{
	classvar documentMap;
	var title, path;
	var <isEdited, <isListener, <envir;

	*initClass {
		documentMap = IdentityDictionary.new;
		Class.initClassTree(EmacsInterface);
		EmacsInterface
		.put(\documentNew, { | id, makeEnvir |
			var doc;
			if (documentMap.includesKey(id).not) {
// 				[\documentNew, id, makeEnvir].postln;
				doc = this.prBasicNew.prInitFromLisp(id);
				if (makeEnvir.notNil) {
					doc.envir = Environment.new;
				};
			};
			nil
		})
		.put(\documentClosed, { | id |
			this.documentDo(id, { | doc |
// 				[\documentClosed, doc].postln;
				doc.closed;
			});
			nil
		})
		.put(\documentSetCurrent, { | id |
			if (current.notNil) {
// 				[\didResignKey, current].postln;
				current.didResignKey;
			};
			if (id.notNil) {
				this.documentDo(id, { | doc |
// 					[\didBecomeKey, doc].postln;
					doc.didBecomeKey;
				});
			};
			nil
		})
		.put(\documentSetProperty, { | id, msg, value |
			this.documentDo(id, { | doc |
// 				[\documentSetProperty, doc, msg, value].postln;
				doc.perform(msg, value);
			});
			nil
		});
	}

	*documentDo { | id, function |
		var doc;
		doc = documentMap.at(id);
		^if (doc.notNil) {
			function.value(doc);
		}
	}
	*documentDoMsg { | id, selector ... args |
		var doc;
		doc = documentMap.at(id);
		^if (doc.notNil) {
			doc.performList(selector, args);
		}
	}

	// lisp support
	storeLispOn { | stream |
		dataptr.storeLispOn(stream)
	}

	// printing
	printOn { | stream |
		super.printOn(stream);
		stream << $( << this.title << $);
	}

	//document setup
	title_ { | argName, completionFunc |
		Emacs.sendToLisp(\_documentRename, [this, argName], {
			completionFunc.value(this);
		});
		super.title_(argName);
	}

	background_ {arg color, rangestart= -1, rangesize = 0;
	}	
	stringColor_ {arg color, rangeStart = -1, rangeSize = 0;
	}

	//interaction:
	front {
		Emacs.sendToLisp(\_documentSwitchTo, this);
	}
	
	unfocusedFront {
		Emacs.sendToLisp(\_documentPopTo, this);
	}
	
	syntaxColorize {
		Emacs.sendToLisp(\_documentSyntaxColorize, this);
	}
	
	selectRange { arg start=0, length=0;
		//_TextWindow_SelectRange
	}
	prisEditable_{ | flag = true |
		Emacs.sendToLisp(\_documentSetEditable, [this, flag]);
	}
	removeUndo{
		Emacs.sendToLisp(\_documentRemoveUndo, this);
	}

	string {arg rangestart, rangesize = 1;
		^""
	}
	
	currentLine {
		^""
	}
	
	// environment support
	envir_ { | environment |
		envir = environment;
		if (this === current) {
			envir.push;
		}
	}

	didBecomeKey {
		if (envir.notNil) {
			envir.push;
		};
		super.didBecomeKey;
	}

	didResignKey {
		if (envir === currentEnvironment) {
			envir.pop;
		};
		super.didBecomeKey;
	}	

	// PRIVATE
	*prNewFromPath { | argPath, selectionStart, selectionLength, completionFunc |
		argPath = this.standardizePath(argPath);
		Emacs.sendToLisp(
			\_documentOpen,
			[argPath, selectionStart + 1, selectionLength],
			{ | id |
				if (id.isNil) {
					"Couldn't create document".warn;
				}{
					this.documentDo(id, completionFunc);
				}
			});
	}
	*prNewFromString { | name, str, makeListener, completionFunc |
		Emacs.sendToLisp(
			\_documentNew,
			[name, str, makeListener],
			{ | id |
				if (id.isNil) {
					"Couldn't create document".warn;
				}{
					this.documentDo(id, completionFunc);
				}
			});
	}
	prInitFromLisp { | id |
		dataptr = id;
		this.prAdd;
	}
	prclose {
		if (dataptr.notNil) {
			Emacs.sendToLisp(\_documentClose, this);
		}
	}
	prAdd {
		allDocuments = allDocuments.add(this);
		documentMap.put(dataptr, this);
		initAction.value(this);
	}
	prRemove {
		allDocuments.remove(this);
		documentMap.removeAt(dataptr);
		dataptr = nil;
	}

	prGetTitle {
		^title
	}
	prSetTitle { | argTitle |
		title = argTitle;
	}
	prGetFileName {
		^path
	}
	prSetFileName { | argPath |
		path = argPath;
		if (path.notNil) {
			path = this.class.standardizePath(path);
		}
	}
	prSetIsListener { | flag |
		isListener = flag.notNil;
	}
	prSetEditable { | flag |
		editable = flag.notNil;
	}
	prSetEdited { | flag |
		isEdited = flag.notNil;
	}

	// unimplemented methods
	prGetBounds { | bounds | ^bounds }
	prSetBounds { }
	setFont { }
	setTextColor { }
	text {
		^""
	}
	selectedText {
		^""
	}
	rangeText { arg rangestart=0, rangesize=1; 
		^""
	}
	prinsertText { arg dataptr, txt;
	}
	insertTextRange { arg string, rangestart, rangesize;
	}
	setBackgroundColor { }	
	selectedRangeLocation {
		^0
	}
	selectedRangeSize {
		^0
	}
	prselectLine { arg line;
	}

	// invalid methods
	initByIndex {
		^this.shouldNotImplement(thisMethod)
	}
	prinitByIndex {
		^this.shouldNotImplement(thisMethod)
	}	
	initLast {
		^this.shouldNotImplement(thisMethod)
	}
	prGetLastIndex {
		^this.shouldNotImplement(thisMethod)
	}
}

// EOF