//create an addressable and resetable single instance of a stream

Pstr : Stream {
	var <>stream, <>pattern;
	
	*new { arg pattern; 
		^this.basicNew(pattern).resetPat;
	}
	
	*basicNew { arg pattern;
		^super.new.pattern_(pattern)
	}
	
	next { arg inval;
			//if(stream.isNil, { this.resetPat }); //in case a nil stream stopped everything
			^stream.next(inval);
	}
	
	reset {
			//pattern.post; "  reset.".postln;
			"_|".post;
			stream.reset;
	}
	
	resetPat {
			//pattern.post; "  resetPat.".postln;
			"_".post;
			stream = pattern.asStream;
	}
	
	refresh { arg pat;
		if(pat.notNil, { pattern = pat; this.resetPat }, { this.reset });
	}
	
}

ResetStream : Pstr {
	var <timeStream, <duration, <>tolerance;
	var <>autoReset=false, <elapsed=0;
	
	*new { arg pattern, dur=1, tolerance=0.001; 
		^super.new(pattern).tolerance_(tolerance).sinit(dur);
	}
	sinit { arg dur;
		if(dur.notNil, { autoReset = true });
		timeStream = dur.asStream;
		duration = timeStream.next;
	}
	
		
	next { arg inval;
		var nextElapsed, event, delta, dur;
		
		if(stream.isNil, { this.resetPat }); //lazy init
		event = stream.next(inval);
		if(event.notNil, {
			delta = event.delta;
			nextElapsed = elapsed + delta;
		
		if(duration.notNil, { 
			if(nextElapsed.round(tolerance) >= duration, {
                    	event.put(\schedMode, \trig); //flag used in StreamLock
                    	//pattern.post; " waiting for trigger...".postln;
                    	elapsed = 0;
                    	duration = timeStream.next;
                    	
 				if(autoReset, { this.resetPat;  });
 				
			}, { elapsed = nextElapsed });
		});
		});
		^event
	}

}

//some more small streams returning a limited number (count) of elements each

Sfin : Stream { 
	*new { arg count, pattern;
		^Pfin(count.asStream, pattern.asStream)
	}
}

Stri : Sfin {
	*new { arg  start=0.0, step=0.125, min=0.0, max=1.0, count=1.0;
		^super.new(count, Ptri(min, max, step, start, inf))
	}
}

Sbrown : Sfin {
	*new { arg  lo=0.0, hi=1.0,  step=0.125, count=1.0;
		^super.new(count, Pbrown(lo, hi, step, inf))
	}

}

Swhite : Sfin {
	*new { arg  lo=0.0, hi=1.0, count=1.0;
		^super.new(count, Pwhite(lo, hi, inf))
	}

}

Srand : Sfin {
	*new { arg  list, count=1.0;
		^super.new(count, Pbrown(list, inf))
	}
}




