
/*
	a player that can switch to different players
	
	doesn't crossfade, this is a raw socket
	
	the players should be in a prepared state
		preparePlayer
		prepareAndTrigger
	
	
	should all be same rate
		numChannels should adapt
*/

PlayerSocket : AbstractPlayerProxy {

	var <>round,<>rate,<>numChannels;
	var <>env;
	
	*new { arg round=0.0,rate=\audio,numChannels=2;
		^super.new.round_(round)
			.rate_(rate).numChannels_(numChannels)
	}
	
	prepareToBundle { arg group,bundle;
		if(source.notNil,{
			source.prepareToBundle(group,bundle)
		})
	}
//	makePatchOut { arg group,public;
//		if(source.notNil,{
//			super.makePatchOut(group,public)
//		})
//	}
	childrenMakePatchOut { arg group,private = true;
		if(source.notNil,{
			source.childrenMakePatchOut(group,private)
		})
	}
	loadDefFileToBundle { arg bundle,server;
		if(source.notNil,{
			source.loadDefFileToBundle(bundle,server)
		})
	}
	spawnToBundle { arg bundle;
		if(source.notNil,{
			source.spawnToBundle(bundle)
		});
		bundle.addMessage(this,\didSpawn);
	}
	instrArgFromControl { arg control;
		^if(this.rate == \audio,{
			In.ar(control,this.numChannels)
		},{
			In.kr(control,this.numChannels)
		})
	}
	
	// prepared sources only
	setSource { arg s,atTime;
		var bundle;
		bundle = CXBundle.new;
		this.setSourceToBundle(s,bundle);
		bundle.send(this.server,atTime);
	}
	setSourceToBundle { arg s,bundle;
		// do replace, same bus
		// set patchout of s ?
		if(source.notNil,{
			//bundle.add( source.stopMsg );
			source.stop;
			
			// deallocate busses !
			// but keep samples etc.
			//source.freePatchOut;
		});
		source = s;
		source.spawnOnToBundle(this.group,this.bus,bundle);
	}

	trigger { arg player,newEnv;
		isSleeping = false;
		// ideally replace the same node, bus
		this.setSource(player);
		this.changed;
	}
	qtime { ^BeatSched.tdeltaTillNext(round) }
	qtrigger { arg player,newEnv,onTrigger;
		var t,bundle;
		// should use a shared BeatSched
		bundle = CXBundle.new;
		
		this.setSourceToBundle(player,bundle);
		bundle.addFunction({
			isSleeping = false;
			this.changed;
			onTrigger.value;
			nil
		});
		bundle.send(this.server, this.qtime );
	}
	
	preparePlayer { arg player;
		// should have prepared the socket first
		player.prepareForPlay(this.group,bus: this.bus)
	}
	prepareAndTrigger { arg player;
		// play on my bus
		player.play(this.group,nil,this.bus);
		isSleeping = false;
		this.changed;
		
		source.stop;

		source = player;
	}

	release {
		isSleeping = true;
		if(source.notNil,{
			source.stop;
		});
		this.changed;
	}
	free {
		isPlaying = false;
		isSleeping = true;
		super.free;
	}
}


//
//PPPregVoice : PlayerSocket {
//
//	var <register;
//	
//	init {
//		register = Array.newClear(3);
//	}
//	setRegister { arg i;
//		register.put(i,)
//
//}




PlayerEffectSocket : PlayerSocket {

	var bus;
	
	setInputBus { arg abus;
		bus = abus.asBus;
		// assume not playing yet
	}	
	
	setSource { arg aplayer;
		aplayer.inputProxies.first.setInputBus(bus);
		super.setSource(aplayer);
	}

}


