
Environment : IdentityDictionary {
	
	*new { arg size=8; ^super.new(size) }
	*make { arg function;
		^this.new.make(function)
	}
	*use { arg function;
		^this.new.use(function)
	}

	make { arg function;
		// pushes the Envir, executes function, returns the Envir
		// usually used to create an environment by adding new variables to it.
		var result, saveEnvir;
		
		saveEnvir = currentEnvironment;
		currentEnvironment = this;
		function.value(this);
		currentEnvironment = saveEnvir;
	}
	use { arg function;
		// temporarily replaces the currentEnvironment with this, 
		// executes function, returns the result of the function
		var result, saveEnvir;
		
		saveEnvir = currentEnvironment;
		currentEnvironment = this;
		result = function.value(this);
		currentEnvironment = saveEnvir;
		^result
	}
	
	eventAt { arg key; ^this.at(key) }	
}

// Events are returned by Pattern Streams

Event : Environment {
	classvar <default;
	
	next { ^this.copy }
	delta {
		_Event_Delta
		^this.primitiveFailed;
		/*
		// implemented by primitive for speed
		var delta;
		delta = this.at('delta');
		if (delta.notNil, { ^delta },{ ^this.at('dur') * this.at('stretch') });
		*/
	}
	play {
		this.at(\player).playEvent(this);
		^this.delta
	}
	
	*initClass {
		Class.initClassTree(Server);
		default = this.make({		
			// fill prototype event with default property values
			
			~reverse = false;
			
			~dur = 1.0;
			~stretch = 1.0;
			~legato = 0.8;
			~sustain = { ~dur * ~legato * ~stretch };
			~lag = 0.0;
			~strum = 0.0;
			
			~amp = { ~db.dbamp };
			~db = -20.0;
			~velocity = 64; 		// MIDI units 0-127
			~pan = 0.0; 			// pan center
			
			~mtranspose = 0;
			~gtranspose = 0.0;
			~ctranspose = 0.0;
			
			~octave = 5.0;
			~root = 0.0;		// root of the scale
			~degree = 0;
			~scale = #[0, 2, 4, 5, 7, 9, 11]; // diatonic major scale
			~stepsPerOctave = 12.0;
			~detune   = 0.0;		// detune in Hertz
			
			~note = {
				(~degree + ~mtranspose).degreeToKey(~scale, ~stepsPerOctave);
			};
			~midinote = {
				var divs;	
				divs = ~stepsPerOctave;
				(~note.value + ~gtranspose + (~octave * divs) + ~root)
					 * 12.0 / divs; 
			};
			~freq = {
				(~midinote.value + ~ctranspose).midicps;
			};

			~instrument = \default;
			
			~group = 0;
			~out = 0;

			// I don't see the point of these.
			// doneAction should be built into the patch.
			// releaseValue should be zero. a negative value would only be for a voice stealer, not a normal note off.
			// why should gate be any value other than 1 ? gate should just be an argument defaulting to one, but not
			// passed in the s_new message.
			//~doneAction = 2;
			//~releaseValue = 0; // sent to gate on release
			//~gate = 1.0;
			
			~finish = {
				// do final calculations
				~freq = ~freq.value + ~detune;
				~amp = ~amp.value;
				~sustain = ~sustain.value;
			};
						
			~player = NotePlayer.new;
			
			~msgFunc = { arg id, freq;
				[[9, ~instrument, id, 1, ~group, 
					\i_out, ~out, \freq, freq, \amp, ~amp, \pan, ~pan]];
			};
			
			~server = Server.local;
		});
		
//		SynthDef(\default, { arg i_out=0, freq=440, amp=0.1, pan=0, gate=1;
//			var z;
//			z = LPF.ar(LFSaw.ar(freq) * Linen.kr(gate, 0.01, amp, 0.3, 2), freq * 12);
//			Out.ar(i_out, Pan2.ar(z, pan));
//		}).writeDefFile;
//
//		SynthDef(\default, { arg i_out=0, freq=440, amp=0.1, pan=0, gate=1;
//			var z;
//			z = RLPF.ar(
//				Mix.ar(LFPulse.ar(freq + [0,Rand(-0.2,0.2),Rand(-0.2,0.2)], 0.1)),
//				XLine.kr(Rand(4000,5000), Rand(2500,3200), 1),
//				0.2) * Linen.kr(gate, 0.01, amp * 0.7, 0.3, 2);
//			Out.ar(i_out, Pan2.ar(z, pan));
//		}).writeDefFile;
		
		SynthDef(\default, { arg i_out=0, freq=440, amp=0.1, pan=0, gate=1;
			var z;
			z = LPF.ar(
				Mix.ar(VarSaw.ar(freq + [0, Rand(-0.4,0.0), Rand(0.0,0.4)], 0, 0.3)),
				XLine.kr(Rand(4000,5000), Rand(2500,3200), 1)) * Linen.kr(gate, 0.01, amp * 0.7, 0.3, 2);
			Out.ar(i_out, Pan2.ar(z, pan));
		}).writeDefFile;

	}
}

