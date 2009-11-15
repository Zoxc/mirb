require 'rake'
require 'rexml/document'

module Builder	
	def self.execute(command, *args)
		#Kernel.puts [command, *args].join(' ')
		IO.popen([command, *args]) do |f|
			f.readlines.join('')
		end
	end
	
	def self.clean(filename)
		if File.exists?(filename)
			File.delete(filename)
			begin
				dir = File.dirname(filename)
				while File.exists?(dir)
					Dir.rmdir(dir)
					dir = File.dirname(dir)
				end
			rescue SystemCallError
			end
		end
	end
	
	def self.require_compiler(settings)
		begin
			self.execute(settings['CC'], '--version')
			true
		rescue
			raise "Unable to find the C compiler: #{settings['CC']}"
		end
	end
	
	def self.puts(setting, *messages)
		Kernel.puts *messages
	end
	
	class Settings
		attr_reader :name, :parent

		def initialize(name, parent = nil, settings = nil)
			@name = name
			@parent = parent
			@settings = settings
		end
		
		def []=(name, value)
			@settings[name] = value
		end
		
		def [](name)
			result = @settings[name]
			parent = (@parent[name] if @parent)
			if result
				if parent
					case result
						when Array
							return result + parent
						when Hash
							return parent.merge(result)
						else
							return result
					end
				else
					return result
				end
			elsif parent
				return parent
			else
				raise "Unable to find setting #{name}"
			end
		end

	end
	
	module Preset
		BASE = Settings.new('Base', nil, {'OUTPUT' => 'build/base', 'CC' => 'gcc', 'CFLAGS' => ['-Wall', '-std=gnu99', '-fstrict-aliasing'], 'LIBS' => [], 'LDFLAGS' => []})
		RELEASE = Settings.new('Release', BASE, {'OUTPUT' => 'build/release', 'CFLAGS' => ['-fexpensive-optimizations', '-O3', '-funroll-loops', '-fomit-frame-pointer'], 'LDFLAGS' => ['-s']})
		DEBUG = Settings.new('Debug', BASE, {'OUTPUT' => 'build/debug', 'CFLAGS' => ['-g', '-DDEBUG']})
	end
	
	class Source
		attr_reader :name, :dependencies, :output
		def initialize(package, name)
			@name = name
			@package = package
			@settings = @package.settings
			@output = File.join(@package.base, @settings['OUTPUT'], @name.ext('.o'))
		end
		
		def respace(str)
			str.gsub(/#{"__&NBSP;__"}/, ' ')
		end
		
		def process_line(line)
			file_tasks, args = line.split(':')
			return if args.nil?
			dependencies = args.split.map { |d| respace(d) }
			file_tasks.strip.split.each do |file_task|
				file_task = respace(file_task)

				@dependencies ||= dependencies
			end
		end
		
		def load_dependencies(node)
			@dependencies = node.elements.map { |dependency| dependency.attribute('name').value }
		end
		
		def get_dependencies(node)	
			Builder.puts @settings, "Getting dependencies for #{@name}..."
			
			lines = Builder.execute(@settings['CC'], *@settings['CFLAGS'], '-MM', '-MT', @output, File.join(@package.base, @name))
			lines.gsub!(/\\ /, "__&NBSP;__")
			lines.gsub!(/#[^\n]*\n/m, "")
			lines.gsub!(/\\\n/, ' ')
			lines.split("\n").each do |line|
				process_line(line)
			end
			
			doc = node.add_element 'source', {'name' => @name, 'time' => File.stat(File.join(@package.base, @name)).mtime}
			
			@dependencies.each do |dependency|
				doc.add_element 'dependency', {'name' => dependency}
			end
		end
		
		def clean
			Builder.clean(@output)
		end
		
		def compile
			if File.exists?(@output)
				mtime = File.stat(@output).mtime
				return false if @dependencies.all? { |dependency| mtime >= File.stat(File.join(@package.base, dependency)).mtime }
			end
			
			makedirs(File.dirname(@output))
			Builder.puts @settings, "Compiling #{@name} for #{@settings.name.downcase}..."
			Builder.execute @settings['CC'], *@settings['CFLAGS'], '-c', File.join(@package.base, @name), '-o', @output
			
			unless File.exist? @output
				raise "Failed to compile file #{@name}."
			end
			
			true
		end
	end
	
	class Package
		attr_reader :name, :base, :settings, :output

		def initialize(name, base, *pattern_list)
			@name = name
			@base = base
			@pattern_list = pattern_list
			@flags = {}
		end
		
		def load_dependencies(file)
			raise unless File.exists?(file)

			xml = REXML::Document.new(File.new(file, 'r'))
			package = xml.root
			raise if package.name != 'package'
			raise if package.attribute('name').value != @name
			
			package.elements.each do |source_node|
				filename = source_node.attribute('name').value
				source = @sources.find { |source| File.identical?(source.name, filename) }
				source.load_dependencies(source_node) if source
			end
			
			raise if @sources.any? { |source| !source.dependencies }
		end
		
		def get_dependencies(file)
			xml = REXML::Document.new
			
			package = xml.add_element 'package', {'name' => @name}
			
			@sources.each { |source| source.get_dependencies(package) }
			
			xml.write(File.new(file, 'w'), 4)
		end
		
		def build_sources
			@sources ||= @pattern_list.map { |source| FileList[source] }.flatten.map { |source| Source.new(self, source) }
		end
		
		def depends_file
			File.join(@base, @settings['OUTPUT'], ".depends.#{@name}.xml")
		end
		
		def compile
			Builder.require_compiler(@settings)

			build_sources
			
			makedirs(File.join(@base, @settings['OUTPUT']))

			begin
				load_dependencies(depends_file)
			rescue
				get_dependencies(depends_file)
			end	
			
			need_linking = false
			
			@sources.each { |source| need_linking = source.compile || need_linking }

			if need_linking || !File.exists?(@output)
				link
				true
			else
				false
			end
		end
		
		def link
			Builder.puts @settings, "Linking #{@name} for #{@settings.name.downcase}..."
			
			Builder.execute @settings['CC'], *@settings['LDFLAGS'], *(@sources.map { |source| source.output }), *(@settings['LIBS'].map { |lib| ['-l', lib] }.flatten), '-o', @output
		end
		
		def clean(settings = Preset::RELEASE)
			@settings = settings
			build_sources
			@sources.each { |source| source.clean }
			Builder.clean(get_output)
			Builder.clean(depends_file)
			true
		end
		
		def get_output
			@output ||= File.join(@base, @settings['OUTPUT'], @name + if Rake::Win32.windows?; '.exe' end.to_s)
		end
		
		def build(settings = Preset::RELEASE)
			@settings = settings
			get_output
			
			compile
		end
	end
end
