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

require_relative 'test_helper'

require 'xmlrpc/client'
require 'xmlrpc/server'

# Monkey patch the xmlrpc server to let us handle regular GET requests.
# Drkonqi partially goes through regular bugzilla cgi's simply requesting xml
# output.
module XMLServerInterceptor
  # raw xml data is here
  # def process(*args)
  #   warn "+++ #{__method__} +++"
  #   p args
  #   warn "--- #{__method__} ---"
  #   super
  # end

  def service(req, resp)
    # Where webrick comes in with request, server rejects non-xmlrpc requests
    # so we'll manually handle GET requests as necessary and forward to an
    # actual bugzilla so we don't have to reimplement everything.
    warn "+++ #{__method__} +++"
    if req.request_method == 'GET'
      if req.request_uri.path.include?('buglist.cgi') # Returns CSV.
        resp.body = <<-EOF
bug_id,"bug_severity","priority","bug_status","product","short_desc","resolution"
375161,"crash","NOR","NEEDSINFO","dolphin","Dolphin crash, copy from Samba share","BACKTRACE"
        EOF
        return
      end

      if req.request_uri.path.include?('show_bug.cgi')
        uri = req.request_uri.dup
        uri.host = 'bugstest.kde.org'
        uri.scheme = 'https'
        uri.port = nil
        resp.set_redirect(WEBrick::HTTPStatus::TemporaryRedirect, uri.to_s)
        return
      end
    end
    warn "--- #{__method__} ---"
    super
  end
end

class XMLRPC::Server
  prepend XMLServerInterceptor

  # Expose webrick server so we can get our port :|
  # https://github.com/ruby/xmlrpc/issues/17
  attr_accessor :server
end

class TestDuplicateAttach < ATSPITest
  def setup
    debug_procs
    server = XMLRPC::Server.new(0)
    port = server.server.config.fetch(:Port)
    ENV['DRKONQI_KDE_BUGZILLA_URL'] = "http://localhost:#{port}/"

    @got_comment = false

    server.set_default_handler do |name, args|
      puts '+++ handler +++'
      p name, args
      if name == 'User.login'
        next {"id"=>12345, "token"=>"12345-cJ5o717AbC"}
      end
      if name == 'Bug.update'
        id = args.fetch('ids').fetch(0)
        cc_to_add = args.fetch('cc').fetch('add')
        next {"bugs"=>[{"last_change_time"=>DateTime.now, "id"=>id, "changes"=>{"cc"=>{"removed"=>"", "added"=>cc_to_add}}, "alias"=>[]}]}
      end
      if name == 'Bug.add_attachment'
        # Check for garbage string from test
        @got_comment = args.fetch('comment').include?('yyyyyyyyyyyyyyyy')
        next { "ids" => [1234] }
      end
      puts '~~~ bugzilla ~~~'
      # Pipe request through bugstest.
      # The arguments are killing me.
      client = XMLRPC::Client.new('bugstest.kde.org', '/xmlrpc.cgi', 443,
                                  nil, nil, nil, nil, true)
      bugzilla = client.call(name, *args)
      p bugzilla
      next bugzilla
    end

    @xml_server_thread = Thread.start { server.serve }

    @tracee = fork { loop { sleep(999_999_999) } }

    assert File.exist?(DRKONQI_PATH), "drkonqi not at #{DRKONQI_PATH}"
    # Will die with our Xephyr in case of errors.
    pid = spawn(DRKONQI_PATH,
                '--signal', '11',
                '--pid', @tracee.to_s,
                '--bugaddress', 'submit@bugs.kde.org',
                '--dialog')
    sleep 4 # Grace to give time to appear on at-spi
    assert_nil Process.waitpid(pid, Process::WNOHANG),
               'drkonqi failed to start or died'
  end

  class ProcInfo
    class Status
      attr_reader :name
      attr_reader :uid
      attr_reader :ppid

      def initialize(path)
        status = read_hash(path)
        @name = status.fetch(:Name, nil)
        @uid = status.fetch(:Uid, nil)[0].to_i
        @ppid = status.fetch(:PPid, 1).to_i
      end

      def read_hash(path)
        hash = {}
        File.read(path).lines do |line|
          ary = line.split(/\n|\t/)
          key = ary.shift
          value = ary.size > 1 ? ary : ary[0]
          value = value.strip if value.respond_to?(:strip)
          hash[key.tr(':', '').to_sym] = value
        end
        hash
      rescue Errno::EACCES
        {}
      end
    end

    attr_reader :pid
    attr_reader :status
    attr_reader :children

    def initialize(pid)
      @pid = pid
      @status = Status.new("/proc/#{pid}/status")
      @children = []
    end

    def cmdline
      File.read("/proc/#{pid}/cmdline").tr("\0", ' ')
    rescue Errno::EACCES
      nil
    end

    def name
      (cmdline || status.name).gsub("\n", "\\n")
    end

    def print_with_children(depth=0)
      indentation = Array.new(depth).join(' ')
      puts "#{indentation}#{name}(#{pid})"
      children.each { |x| x.print_with_children(depth + 2) }
    end
  end

  def debug_procs
    infos = Dir.glob('/proc/*').collect do |x|
      pid = File.basename(x)
      next unless pid =~ /\d+/
      pid = pid.to_i
      next if pid == Process.pid
      [pid, ProcInfo.new(pid)]
    end.compact.to_h
    infos.each do |_pid, info|
      next if info.status.ppid.zero? # kernelish processes
      begin
        infos.fetch(info.status.ppid).children << info
      rescue KeyError
        # either dangling or died already
      end
    end
    infos.fetch(1).print_with_children
  end

  def teardown
    unless passed?
      debug_procs
    end

    Process.kill('KILL', @tracee)
    Process.waitpid2(@tracee)
    @xml_server_thread.kill
    @xml_server_thread.join
  end

  # When evaluating duplicates
  def test_duplicate_attach
    drkonqi = ATSPI.desktops[0].applications.find { |x| x.name == 'drkonqi' }
    refute_nil drkonqi, 'Could not find drkonqi on atspi api.' \
                        ' Maybe drkonqi is not running (anymore)?'

    accessible = find_in(drkonqi.windows[-1], name: 'Report Bug')
    press(accessible)

    find_in(drkonqi, name: 'Crash Reporting Assistant') do |window|
      accessible = find_in(window, name: 'Next')
      press(accessible)

      accessible = find_in(window, name: 'Yes')
      toggle_on(accessible)

      accessible = find_in(window, name: /^What I was doing when the application.+/)
      toggle_on(accessible)

      accessible = find_in(window, name: 'Next')
      press(accessible)

      loop do
        # Drkonqi is now doing the trace, wait until it is done.
        accessible = find_in(window, name: 'Next')
        refute_nil accessible
        if accessible.states.include?(:sensitive)
          press(accessible)
          break
        end
        warn accessible.states
        sleep 2
      end

      # Set pseudo login data if there are none.
      accessible = find_in(window, name: 'Username input')
      accessible.text.set_to 'xxx' if accessible.text.length <= 0
      accessible = find_in(window, name: 'Password input')
      accessible.text.set_to 'yyy' if accessible.text.length <= 0

      accessible = find_in(window, name: 'Login')
      press(accessible)

      sleep 2 # Wait for login and bug listing

      accessible = find_in(window, name: '375161')
      toggle_on(accessible)

      accessible = find_in(window, name: 'Open selected report')
      press(accessible)
    end

    find_in(drkonqi, name: 'Bug Description') do |window|
      accessible = find_in(window, name: 'Suggest this crash is related')
      press(accessible)
    end

    find_in(drkonqi, name: 'Related Bug Report') do |window|
      accessible = find_in(window, name: /^Completely sure: attach my information.+/)
      toggle_on(accessible)

      accessible = find_in(window, name: 'Continue')
      press(accessible)
    end

    find_in(drkonqi, name: 'Crash Reporting Assistant') do |window|
      accessible = find_in(window, name: /^The report is going to be attached.+/)
      refute_nil accessible

      accessible = find_in(window, name: 'Next')
      press(accessible)

      accessible = find_in(window, name: 'Information about the crash text')
      accessible.text.set_to(accessible.text.to_s +
        Array.new(128).collect { 'y' }.join)

      accessible = find_in(window, name: 'Next')
      press(accessible)

      accessible = find_in(window, name: 'Next')
      press(accessible)

      accessible = find_in(window, name: /.*Crash report sent.*/)
      refute_nil accessible

      accessible = find_in(window, name: 'Finish')
      press(accessible)
    end

    assert @got_comment # only true iff the server go tour yyyyyy garbage string
  end
end
