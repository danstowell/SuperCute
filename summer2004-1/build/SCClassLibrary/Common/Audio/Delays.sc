
Delay1 : UGen {
	
	*ar { arg in = 0.0, mul = 1.0, add = 0.0;
		^this.multiNew('audio', in).madd(mul, add)
	}
	*kr { arg in = 0.0, mul = 1.0, add = 0.0;
		^this.multiNew('control', in).madd(mul, add)
	}
}

Delay2 : Delay1 { }

///////////////////////////////////////

// these delays use real time allocated memory.

DelayN : UGen {
	
	*ar { arg in = 0.0, maxdelaytime = 0.2, delaytime = 0.2, mul = 1.0, add = 0.0;
		^this.multiNew('audio', in, maxdelaytime, delaytime).madd(mul, add)
	}
	*kr { arg in = 0.0, maxdelaytime = 0.2, delaytime = 0.2, mul = 1.0, add = 0.0;
		^this.multiNew('control', in, maxdelaytime, delaytime).madd(mul, add)
	}
}

DelayL : DelayN { }
DelayC : DelayN { }


CombN : UGen {
	
	*ar { arg in = 0.0, maxdelaytime = 0.2, delaytime = 0.2, decaytime = 1.0, mul = 1.0, add = 0.0;
		^this.multiNew('audio', in, maxdelaytime, delaytime, decaytime).madd(mul, add)
	}
	*kr { arg in = 0.0, maxdelaytime = 0.2, delaytime = 0.2, decaytime = 1.0, mul = 1.0, add = 0.0;
		^this.multiNew('control', in, maxdelaytime, delaytime, decaytime).madd(mul, add)
	}
}

CombL : CombN { }
CombC : CombN { }

AllpassN : CombN { }
AllpassL : CombN { }
AllpassC : CombN { }

///////////////////////////////////////

// these delays use shared buffers.

BufDelayN : UGen {
	
	*ar { arg buf = 0, in = 0.0, delaytime = 0.2, mul = 1.0, add = 0.0;
		^this.multiNew('audio', buf, in, delaytime).madd(mul, add)
	}
	*kr { arg buf = 0, in = 0.0, delaytime = 0.2, mul = 1.0, add = 0.0;
		^this.multiNew('control', buf, in, delaytime).madd(mul, add)
	}
}

BufDelayL : BufDelayN { }
BufDelayC : BufDelayN { }


BufCombN : UGen {
	
	*ar { arg buf = 0, in = 0.0, delaytime = 0.2, decaytime = 1.0, mul = 1.0, add = 0.0;
		^this.multiNew('audio', buf, in, delaytime, decaytime).madd(mul, add)
	}
}

BufCombL : BufCombN { }
BufCombC : BufCombN { }

BufAllpassN : BufCombN { }
BufAllpassL : BufCombN { }
BufAllpassC : BufCombN { }

///////////////////////////////////////

/*
GrainTap : MultiOutUGen {	
	*ar { arg grainDur = 0.2, pchRatio = 1.0, 
			pchDispersion = 0.0, timeDispersion = 0.0, overlap = 2.0, mul = 1.0, add = 0.0;
		^this.multiNew('audio', grainDur, pchRatio, 
			pchDispersion, timeDispersion, overlap).madd(mul, add)
	}
}
*/

