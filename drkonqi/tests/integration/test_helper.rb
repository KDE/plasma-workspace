# Copyright (C) 2017 Harald Sitter <sitter@kde.org>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License or (at your option) version 3 or any later version
# accepted by the membership of KDE e.V. (or its successor approved
# by the membership of KDE e.V.), which shall act as a proxy
# defined in Section 14 of version 3 of the license.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

STDOUT.sync = true # force immediate flushing without internal caching

DRKONQI_PATH = ENV['DRKONQI_PATH']
AT_SPI_BUS_LAUNCHER_PATH = ENV['AT_SPI_BUS_LAUNCHER_PATH']
warn "Testing against #{DRKONQI_PATH} with #{AT_SPI_BUS_LAUNCHER_PATH}"

# Dies together with our dbus.
# Make sure to enable a11y and screen-reader explicitly so qt definitely
# activates its a11y bindings.
spawn(AT_SPI_BUS_LAUNCHER_PATH,
      '--launch-immediately',
      '--a11y=1',
      '--screen-reader=1')

require 'atspi'
require 'minitest/autorun'

# Adds convenience methods for ATSPI on top of minitest.
class ATSPITest < Minitest::Test
  def find_in(parent, name: nil, recursion: false)
    raise 'no accessible' if parent.nil?
    accessibles = parent.children.collect do |child|
      ret = []
      if child.children.size != 0 # recurse
        ret += find_in(child, name: name, recursion: true)
      end
      if name && child.states.include?(:showing)
        if (name.is_a?(Regexp) && child.name.match(name)) ||
           (name.is_a?(String) && child.name == name)
          ret << child
        end
      end
      ret
    end.compact.uniq.flatten
    return accessibles if recursion
    raise "not exactly one accessible for #{name} => #{accessibles.collect {|x| x.name}.join(', ')}" if accessibles.size > 1
    raise "cannot find accessible(#{name})" if accessibles.size < 1
    yield accessibles[0] if block_given?
    accessibles[0]
  end

  def press(accessible)
    raise 'no accessible' if accessible.nil?
    action = accessible.actions.find { |x| x.name == 'Press' }
    refute_nil action, 'expected accessible to be pressable'
    action.do_it!
    sleep 0.25
  end

  def focus(accessible)
    raise 'no accessible' if accessible.nil?
    action = accessible.actions.find { |x| x.name == 'SetFocus' }
    refute_nil action, 'expected accessible to be focusable'
    action.do_it!
    sleep 0.1
  end

  def toggle(accessible)
    raise 'no accessible' if accessible.nil?
    action = accessible.actions.find { |x| x.name == 'Toggle' }
    refute_nil action, 'expected accessible to be toggle'
    action.do_it!
    sleep 0.1
  end

  def toggle_on(accessible)
    raise 'no accessible' if accessible.nil?
    return if accessible.states.any? { |x| %i[checked selected].include?(x) }
    toggle(accessible)
  end
end
