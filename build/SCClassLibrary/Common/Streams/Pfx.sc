Pfx : FilterPattern {
	var <>fxname, <>pairs;
	*new { arg pattern, fxname ... pairs;
		if (pairs.size.odd, { Error("Pfx should have even number of args.\n").throw });
		^super.new(pattern).fxname_(fxname).pairs_(pairs)
	}
	storeArgs { ^[pattern, fxname] ++ pairs }
	embedInStream { arg inevent;	
		var stream, cleanup = EventStreamCleanup.new;
		var server = inevent[\server] ?? { Server.default };
		var id = server.nextNodeID;
		var event = inevent.copy;
		
		pairs.pairsDo {|name, value|
			event[name] = value;
		};
		event[\addAction] = 1; // add to tail of group.
		event[\instrument] = fxname;
		event[\type] = \on;
		event[\id] = id;
		event[\delta] = 0;
		
		cleanup.addFunction(event, { |flag| 
			if (flag) { (type: \off, id: id).play } 
		});		
		
		inevent = event.yield;
		
		stream = pattern.asStream;
		
		loop {
			event = stream.next(inevent) ?? { ^cleanup.exit(inevent) };
			cleanup.update(event);
			inevent = event.yield;
		};
	}
}



Pgroup : FilterPattern {

	embedInStream { arg inevent;
	
		var server, groupID, event, ingroup, cleanup;
		var stream, lag;
		
		cleanup = EventStreamCleanup.new;
		server = inevent[\server] ?? { Server.default };
		ingroup = inevent[\group];		
		groupID = server.nextNodeID;
		
		event = inevent.copy;
		event[\addAction] = 1;
		event[\type] = \group;
		event[\delta] = 1e-9; // no other sync choice for now. (~ 1 / 20000 sample delay)
		event[\id] = groupID;
		event[\group] = ingroup;
		cleanup.addFunction(event, { | flag |
			if (flag) { (lag: lag, type: \kill, id: groupID, server: server).play }
		});
		inevent = event.yield;
		
		inevent !? { inevent = inevent.copy; inevent[\group] = ingroup };
//		^this.class.embedLoop(inevent, pattern.asStream, groupID, ingroup, cleanup);
		stream = pattern.asStream;
		 loop {
			event = stream.next(inevent) ?? { ^cleanup.exit(inevent) };			lag = event.use { ~sustain.value + 0.1};	
			inevent = event.yield;
			inevent.put(\group, groupID); 
		}
		
	}

	*embedLoop { arg inevent, stream, groupID, ingroup, cleanup;
		var event, lag;
		 loop {
			event = stream.next(inevent) ?? { ^cleanup.exit(inevent) };			lag = event[\dur];	
			inevent = event.yield;
			inevent.put(\group, groupID); 
		}
	}

	
}

Pbus : FilterPattern {
	var <>numChannels, <>rate, <>dur=2.0, <>fadeTime;
	
	*new { arg pattern, dur=2.0, fadeTime=0.02, numChannels=2, rate=\audio;
		^super.new(pattern).dur_(dur).numChannels_(numChannels).rate_(rate).fadeTime_(fadeTime)
	}
	
	storeArgs { ^[ pattern, dur, fadeTime, numChannels, rate ] }
	
	embedInStream { arg inevent;
		var server, groupID, linkID, bus, ingroup, cleanup;
		var patterns, event, freeBus, stream;
		
		cleanup = EventStreamCleanup.new;
		server = inevent[\server] ?? { Server.default };
		groupID = server.nextNodeID;
		linkID = server.nextNodeID;
		ingroup = inevent[\group];
		
		// could use a special event type for this:
		if(rate == \audio) {
			bus = server.audioBusAllocator.alloc(numChannels);
			freeBus = { server.audioBusAllocator.free(bus) };
		} {
			bus = server.controlBusAllocator.alloc(numChannels);
			freeBus = { server.controlBusAllocator.free(bus) };
		};
		
		event = inevent.copy;
		event[\addAction] = 1;
		event[\type] = \group;
		event[\delta] = 1e-9;
		event[\id] = groupID;
		event[\group] = ingroup;
		event.yield;
		
		
		inevent = event = inevent.copy;
		
		event[\type] = \on;
		event[\group] = groupID;
		event[\addAction] = 3;
		event[\delta] = 1e-9;
		event[\id] = linkID;
		event[\fadeTime] = fadeTime;
		event[\instrument] = format("system_link_%_%", rate, numChannels);
		event[\in] = bus;

		cleanup.addFunction(event, { | flag |
			if(flag) { (id: linkID, type: \off, gate: dur.neg, hasGate: true).play }; 
		});

		cleanup.addFunction(event, { freeBus.value; });
		
		// doneAction = 3;   
		// remove and deallocate both this synth and the preceeding node 
		// (which is the group).
		
		event[\msgFunc] = #{ |out, in, fadeTime, gate=1|
			[\out, out, \in, in, \fadeTime, fadeTime, \gate, gate, \doneAction, 3] 
		};
		
		inevent = event.yield;
		
		
		// now embed the pattern
		
		stream = Pchain(pattern, (group: groupID, out: bus)).asStream;
		
		
		loop {
			
			
			event = stream.next(inevent) ?? { ^cleanup.exit(inevent) };
			cleanup.update(event);
			inevent = event.yield;
			
			
		}
	}
	
	
}

