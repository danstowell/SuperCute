//todo: make efficient by using PriorityQueue

Order : SequenceableCollection {
	var <>array, <>indices;
	
	*new { arg size = 8; 
		^super.new.clear(size)
	}
	
	*with { arg obj;
		^this.new.put(0, obj)
	}
	
	clear { arg size = 8;
		array = Array.new(size);
		indices = Array.new(size);
	}
	
	add { arg obj;
			var index;
			index = if(indices.isEmpty, { 0 }, { indices.last  + 1 });
			array = array.add(obj);
			indices = indices.add(index);
	}
	
		
	removeAt { arg index;
		var realIndex;
		realIndex = this.slotOf(index);
		^if(realIndex.notNil, {
			indices.removeAt(realIndex);
			array.removeAt(realIndex);
		}, {
			nil
		})
	}
	
	remove { arg item;
		var index, res;
		index = array.indexOf(item);
		^if(index.notNil, {
			indices.removeAt(index);
			res = array.removeAt(index);
		});
	}
	
	pop {
		indices.pop;
		^array.pop
	}
	
	at { arg index;
		var realIndex;
		realIndex = this.slotOf(index);
		^if(realIndex.notNil, {
			array.at(realIndex)
		})
	}
	
	slotOf { arg i;
		(i+1).do({ arg j; if(indices.at(j) === i, { ^j }) });
		^nil
	}

	nextSlot { arg index;
		if(indices.notEmpty and: { index <= indices.last }, {
			indices.do({ arg item, j; if(item >= index, { ^j }) });
		});
		^nil
	}
	
	put { arg index, obj;
		var nextIndex, nextSlot;
		
		nextSlot = this.nextSlot(index);
		if(nextSlot.isNil, {
			array = array.add(obj);
			indices = indices.add(index);
		}, {
			nextIndex = indices.at(nextSlot);
			if(nextIndex === index, {
				array.put(nextSlot, obj) //replace existing object
			}, {
				array = array.insert(nextSlot, obj); //insert into order
				indices = indices.insert(nextSlot, index);
			})
		})
			
	}
	
	copy {
		 ^this.class.newCopyArgs(array.copy, indices.copy)
	}
	
	do { arg function;
		array.do(function)
	}
	
		
	collect { arg func;
		var res;
		res = this.class.new.indices_(indices.copy);
		res.array = array.collect(func);
		^res
	}
	
	select { arg func;
		^this.copy.selectInPlace(func);
	}
	
	reject { arg func;
		^this.copy.rejectInPlace(func);
	}
	
	collectInPlace { arg function;
		array = array.collect(function)
	}
	
	selectInPlace { arg function;
		array = array.select({ arg elem, i; 
			var bool;
			bool = function.value(elem,i);
			if(bool.not, { indices.removeAt(i) });
			bool
		});
	}
	
	rejectInPlace { arg function;
		array = array.reject({ arg elem, i; 
			var bool;
			bool = function.value(elem,i);
			if(bool, { indices.removeAt(i) });
			bool
		});
	}
	

}




/*
ProxyOrder - play to group
	- keine nodemap
	- add / replace etc..
*/


