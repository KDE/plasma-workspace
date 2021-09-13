#!/usr/bin/env ruby
# frozen_string_literal: true

# SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
# SPDX-FileCopyrightText: 2021 Harald Sitter <sitter@kde.org>

# Generates timezone tables out of tzdata.zi backing data such that the pretty names may be translated.

require 'erb'
require 'ostruct'

# Wraps template rendering. Templating is implemented using the ERB template system.
class Template
  def self.render(context)
    erb = ERB.new(File.read("#{__dir__}/timezonesi18n_generated.h.erb"))
    erb.result(context.instance_eval { binding })
  end
end

# Context object for template rendering. Inside the template file anything defined in here may be used.
# For simplicity's sake continents and cities are not modeled as fully functional objects but rather use
# this context wrapper. Slightly less code.
class RenderContext < OpenStruct
  def initialize(continents:, cities:)
    super
  end

  def city_description(city)
    return 'This is a generic time zone name, localize as needed' if city.start_with?('UTC')

    'This is a city associated with particular time zone'
  end

  def continent_description(_continent)
    'This is a continent/area associated with a particular timezone'
  end

  # prettify city name
  def to_city_name(city)
    city = city.tr('_', ' ')
    {
      'DumontDUrville' => 'Dumont d’Urville',
      'ComodRivadavia' => 'Comodoro Rivadavia'
      # TODO do we want these? seeing as different transliterations are in use, one is just as bad as the other, surely
      # 'Uzhgorod' => 'Uzhhorod' # the tz has the legacy transliteration
      # 'Zaporozhye' => 'Zaporizhzhia'
      # 'Kiev' => 'Kyiv'
    }.fetch(city, city)
  end

  # prettify continent name
  def to_contient_name(continent)
    continent # leave unchanged, they are all lovely
  end
end

raise 'tzdata.zi missing, your tzdata was possibly built without' unless File.exist?('/usr/share/zoneinfo/tzdata.zi')

data = File.read('/usr/share/zoneinfo/tzdata.zi')

continents = []
cities = []
data.split("\n").each do |line|
  line = line.strip
  next if line[0] == '#' # comment

  # zi files have a tricky format. only look at lines that make sense for us (Zone entries and Link entries)
  is_zone = line[0].downcase == 'z'
  is_link = line[0].downcase == 'l'
  next unless is_zone || is_link

  line_parts = line.split(' ')

  # Since we have 2 line formats, the location of the timezone is different in each
  tz = is_zone ? line_parts[1] : line_parts[2]
  parts = tz.split('/')

  # some links are fishy single names we can't use (e.g. `L Europe/Warsaw Poland`)
  next if parts.size < 2

  continent = parts[0]

  # some links also have fake continent values (e.g. `L America/Denver SystemV/MST7MDT`) followed by regions therein
  next if %w[systemv us etc canada brazil mexico chile].any? { |x| x == continent.downcase }

  city = parts[-1]

  # some links are simply pointing at directions within a region/country
  next if %w[north east south west].any? { |x| x == city.downcase }
  # some links are abbreviations (e.g. `Australia/NSW` for new south wales)
  # Australia has some more links for regions but we'd have to filter them individually and I can't be bothered.
  next if city.size <= 3 && city.upcase == city
  # some are Knox and also known as Knox_IN for reasons
  next if city == 'Knox_IN'

  continents << continent
  cities << city
end

# make sure fake cities exist
(0..14).each do |i|
  cities << format('UTC+%02d:00', i)
  cities << format('UTC-%02d:00', i)
end
# The partial hours are from original code this script was built based on. They do not actually correspond to timezone
# links though, so I'm not sure how they work but keep them regardless since I hope there's magic going on somehwere.
cities += %w[UTC UTC+03:30 UTC+04:30 UTC+05:30 UTC+05:45 UTC+06:30 UTC+09:30 UTC-03:30 UTC-04:30]

cities = cities.sort_by(&:downcase).uniq
continents = continents.sort_by(&:downcase).uniq

data = Template.render(RenderContext.new(cities: cities, continents: continents))
File.write("#{__dir__}/timezonesi18n_generated.h", data)
