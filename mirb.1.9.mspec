# Configuration file for Mirb

class MSpecScript
  # Language features specs
  set :language, [ 'rubyspec/language' ]

  # Core library specs
  set :core, [
    'rubyspec/core',
	
    '^rubyspec/core/basicobject',
    '^rubyspec/core/argf'
  ]
  
  # An ordered list of the directories containing specs to run
  set :files, get(:language) + get(:core)

  # This set of files is run by mspec ci
  set :ci_files, get(:files)

  set :tags_patterns, [
                        [%r(language/),     'rubyspec/tags/1.9/language/'],
                        [%r(core/),         'rubyspec/tags/1.9/core/'],
                        [/_spec.rb$/,       '_tags.txt']
                      ]
end
