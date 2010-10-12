require_relative '../reno-0.2/reno'

include Reno

output = 'mirb'

package = Package.new do
	# name and version
	name 'mirb'
	version '0.1.0'

	windows = Rake::Win32.windows?
	debug = true
	
	# setup toolchain
	
	set Toolchain::Architecture, Arch::X86

	set Toolchain::Optimization, :balanced
	set Toolchain::Exceptions, :none
	set Toolchain::StaticLibraries, true

	set Toolchain::Libraries, 'vendor/gc/.libs/gc'
	set Toolchain::Libraries, 'vendor/udis86/libudis86/.libs/udis86'

	set Toolchain::Libraries, 'pthread' unless windows
	
	set Languages::C::Includes, ['vendor', 'vendor/udis86']
	
	use Toolchain::GNU
	
	# languages
	use Assembly::WithCPP
	c = use Languages::C
	c.std 'c99'
	c.define('WIN_SEH') if windows
	c.define 'DEBUG' if debug
	cxx = use Languages::CXX
	cxx.std 'c++0x'
	
	# files
	files = collect('main.c', 'compiler/**/*.c', 'runtime/**/*.c')

	files += collect('runtime/**/*.S') unless debug
	
	files.merge(Executable).name(output, windows)
end

task :build do
	package.run
end

task :default => :build
