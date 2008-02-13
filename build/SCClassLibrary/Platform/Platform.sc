Platform
{
	var <classLibraryDir, <helpDir, <>recordingsDir, features;

	initPlatform {
		classLibraryDir = thisMethod.filenameSymbol.asString.dirname.dirname;
		helpDir = thisMethod.filenameSymbol.asString.dirname.dirname.dirname ++ "/Help";
		features = IdentityDictionary.new;
		recordingsDir = this.userAppSupportDir +/+ "Recordings";
	}

	name { ^this.subclassResponsibility }
	*case { | ... cases |
		^thisProcess.platform.name.switch(*cases)
	}

	// directories
	*classLibraryDir { ^thisProcess.platform.classLibraryDir }
	*helpDir { ^thisProcess.platform.helpDir }

	systemAppSupportDir { _Platform_systemAppSupportDir }
	*systemAppSupportDir { ^thisProcess.platform.systemAppSupportDir }

	userAppSupportDir { _Platform_userAppSupportDir }
	*userAppSupportDir { ^thisProcess.platform.userAppSupportDir }

	systemExtensionDir { _Platform_systemExtensionDir }
	*systemExtensionDir { ^thisProcess.platform.systemExtensionDir }

	userExtensionDir { _Platform_userExtensionDir }
	*userExtensionDir { ^thisProcess.platform.userExtensionDir }

	platformDir { ^this.name.asString }
	*platformDir { ^thisProcess.platform.platformDir }
	
	pathSeparator { ^this.subclassResponsibility }
	*pathSeparator { ^thisProcess.platform.pathSeparator }
	
	// startup/shutdown hooks
	startup { }
	shutdown { }

	startupFiles { ^#[] }
	loadStartupFiles { this.startupFiles.do(_.loadPaths) }

	// features
	declareFeature { | aFeature |
		var str = aFeature.asString;
		if (str.first.isUpper) {
			Error("cannot declare class name features").throw;			
		};
		if (str.first == $_) {
			Error("cannot declare primitive name features").throw;
		};
		features.put(aFeature, true);
	}
	hasFeature { | symbol |
		if (features.includesKey(symbol).not) {
			features.put(
				symbol,
				symbol.asSymbol.asClass.notNil or: { symbol.isPrimitive }
			)
		};
		^features.at(symbol)
	}
	when { | features, ifFunction, elseFunction |
		^features.asArray.inject(true, { |v,x|
			v and: { this.hasFeature(x) }
		}).if(ifFunction, elseFunction)
	}
	*when {  | features, ifFunction, elseFunction |
		^thisProcess.platform.when(features, ifFunction, elseFunction)
	}

	// swing is compatible with all platforms; so declare it as global default
	defaultGUIScheme { ^\swing }
	defaultHIDScheme { ^\none }
}

UnixPlatform : Platform
{
	pathSeparator { ^$/ }
	
	arch {
		var pipe, arch;
		pipe = Pipe("arch", "r");
		arch = pipe.getLine;
		pipe.close;
		^arch.asSymbol;
	}
}
