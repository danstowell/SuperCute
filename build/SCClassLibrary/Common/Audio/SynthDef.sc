
SynthDef {	
	var <>name;
	var <>controlNames;
	var <>controls;
	var <>children;
		
	var <>constants;
	var <>constantSet;
	
	// topo sort
	var <>arAvailable, <>krAvailable;
	
	*new { arg name, ugenGraphFunc;
		^super.new.name_(name.asString).build(ugenGraphFunc);
	}
	
	addUGen { arg ugen;
		ugen.synthIndex = children.size;
		children = children.add(ugen);
	}
	removeUGen { arg ugen;
		children.remove(ugen);
		this.indexUGens;
	}
	
	build { arg func;
		var controls;
		
		UGen.buildSynthDef = this;
		constants = Dictionary.new;
		constantSet = Set.new;
		
		controls = this.buildControlsFromArgs(func);
		func.valueArray(controls);

		this.optimizeGraph;

		if (this.checkInputs.not, { ^nil });
		this.collectConstants;
		
		// re-sort graph. reindex.
		this.topologicalSort;
		this.indexUGens;
	}
	buildControlsFromArgs { arg func;
		var def, size, names, values, controls;
		var irnames, irvalues, ircontrols, irpositions;
		var krnames, krvalues, krcontrols, krpositions;

		def = func.def;
		names = def.argNames;
		if (names.isNil, { ^nil });
		
		// OK what we do here is separate the ir and kr rate arguments,
		// create one Control ugen for all of the ir and one for all of 
		// the kr, and then construct the argument array from combining 
		// the OutputProxies of these two Control ugens in the original order.
		size = names.size;
		values = def.prototypeFrame.copy.extend(size);
		values = values.collect({ arg value; value ? 0.0 });
		names.do({ arg name, i; 
			var c, c2, value;
			#c, c2 = name.asString;
			value = values.at(i);
			if (c == $i && { c2 == $_ }, {
				irnames = irnames.add(name);
				irvalues = irvalues.add(value);
				irpositions = irpositions.add(i);
			},{
				krnames = krnames.add(name);
				krvalues = krvalues.add(value);
				krpositions = krpositions.add(i);
			});
		});
		if (irnames.size > 0, {
			ircontrols = Control.names(irnames).ir(irvalues);
		});
		if (krnames.size > 0, {
			krcontrols = Control.names(krnames).kr(krvalues);
		});
		controls = Array.newClear(size);
		ircontrols.asArray.do({ arg control, i; 
			controls.put(irpositions.at(i), control);
		});
		krcontrols.asArray.do({ arg control, i; 
			controls.put(krpositions.at(i), control);
		});
		
		^controls
	}
	
	checkInputs {
		children.do({ arg ugen; 
			if (ugen.checkInputs.not, { 
				Post << ugen.class << " has bad inputs: " 
					<< ugen.inputs << ".\n";
				^false 
			});
		});
		^true
	}
	
	addConstant { arg value;
		if (constantSet.includes(value).not, {
			constantSet.add(value);
			constants.put(value, constants.size);
		});
	}
	collectConstants {
		children.do({ arg ugen; 
			ugen.collectConstants;
		});
	}

	asBytes {
		var stream;
		stream = CollStream.on(Int8Array.new(256));
		this.asArray.writeDef(stream);
		^stream.collection;
	}
	writeDefFile { arg inName;
		super.writeDefFile(inName ? name)
	}
	writeDef { arg file;
		// This describes the file format for the synthdef files.
		
		file.putPascalString(name);
		
		this.writeConstants(file);

		file.putInt16(controls.size);
		controls.do({ arg item;
			file.putFloat(item.asFloat);
		});
				
		file.putInt16(controlNames.size);
		controlNames.do({ arg item;
			if (item.name.notNil, {
				file.putPascalString(item.name);
				file.putInt16(item.index);
			});
		});

		file.putInt16(children.size);
		children.do({ arg item;
			item.writeDef(file);
		});
	}
	writeConstants { arg file;
		var array;
		array = Array.newClear(constants.size);
		constants.keysValuesDo({ arg value, index;
			array.put(index, value);
		});

		//constants.size.postln;
		//array.postln;

		file.putInt16(constants.size);
		array.do({ arg item; 
			file.putFloat(item) 
		});
	}

	// multi channel expansion causes a non optimal breadth-wise ordering of the graph.
	// the topological sort below follows branches in a depth first order,
	// so that cache performance of connection buffers is optimized.
	
	optimizeGraph {
		this.initTopoSort;
		children.copy.do({ arg ugen;
			ugen.optimizeGraph;
		});
	}
	
	replaceUGen { arg a, b;
		children.do({ arg item, i;
			if (item === a, { children.put(i, b) });
			item.inputs.do({ arg input, j;
				if (input === a, { item.inputs.put(j, b) });
			});
		});
	}
	
	initTopoSort {
		arAvailable = krAvailable = nil;
		children.do({ arg ugen;
			ugen.antecedents = Set.new;
			ugen.descendants = Set.new;
		});
		children.do({ arg ugen;
			// this populates the descendants and antecedents
			ugen.initTopoSort;
		});
		children.do({ arg ugen;
			ugen.descendants = ugen.descendants.asSortedList;
			ugen.makeAvailable; // all ugens with no antecedents are made available
		});
	}
	topologicalSort {
		var outStack;
		this.initTopoSort;
		while ({ krAvailable.size + arAvailable.size > 0 },{
			while ({ krAvailable.size > 0 },{
				outStack = krAvailable.pop.schedule(outStack);
			});
			if (arAvailable.size > 0, {
				outStack = arAvailable.pop.schedule(outStack);
			});
		});
		
		children = outStack;
	}
	indexUGens {
		children.do({ arg ugen, i;
			ugen.synthIndex = i;
		});
	}
	
	dumpUGens {
		name.postln;
		children.do({ arg ugen, i;
			var inputs;
			if (ugen.inputs.notNil, {
				inputs = ugen.inputs.collect({ arg in; 
					if (in.isKindOf(UGen), { in.source },{ in })
				});
			});
			[i, ugen, ugen.rate, ugen.synthIndex, inputs].postln;
		});
	}
	
	load { arg server, completionMsg;
		this.writeDefFile;
		server.loadSynthDef(name, completionMsg);
	}
	
	play { arg target,args,addAction=\addToTail;
		var synth, msg;
		target = target.asTarget;
		synth = Synth.prNew(name);
		msg = synth.newMsg(target, addAction, args);
		this.load(target.server, msg);
		^synth
	}
}

