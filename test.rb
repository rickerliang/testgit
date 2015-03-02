#!/usr/bin/env ruby
#encoding=UTF-8 

puts "----------rubytest loaded---------------"

require 'win32api'
require 'set.rb'

State = {'0' => 'Uninit', '1' => 'Initialized', '2' => 'Loading', '3' => 'Loaded', '4' => 'Invalid'}

class String
  def to_path(end_slash=false)
    "#{'/' if self[0]=='\\'}#{self.split('\\').join('/')}#{'/' if end_slash}" 
  end 
end

def main
	puts "----------rubytest main---------------"
#	puts Encoding.default_internal
#	puts Encoding.default_external

	pluginMgr =  $kxApp.getPluginMgr
	puts pluginMgr.class
	puts pluginMgr.loaded
	pluginCount = pluginMgr.count
	puts 'plugin count ' + pluginCount.to_s
	
	plugins = []
	for i in 0..pluginCount-1
		plugins << pluginMgr.getPluginByIdx(i)
	end
	out_put_plugins_info(plugins)
	check_plugins_state(plugins)
	puts "----------rubytest complete-------------"
end

class PluginLoadingObserver < Qt::Object
	slots 'loadSuccess()', 'loadFailed()', 'loadProgress(long long, long long)'
	def initialize(parent, plugin)
		super(parent)
		@plugin = plugin
		
		connect(@plugin, SIGNAL('loadSuccess()'), SLOT('loadSuccess()'))
		connect(@plugin, SIGNAL('loadFailed()'), SLOT('loadFailed()'))
	end
	
	def loadSuccess()
		check_plugin_state(@plugin)
	end
	
	def loadFailed()
		puts 'force load fail @@@@@@@@ ' + @plugin.getProp('name').toString
	end
end

def check_plugin_module(plugin)
	modules = retrieve_process_module_snapshot
	path = plugin.localPath.to_s + '/' + plugin.getProp("name").toString + '.dll'
	path = path.downcase().to_path().encode('UTF-8')
	modules.each do |modulePath|
#		puts modulePath.encoding()
		if modulePath.eql?(path)
			puts 'check module ' + plugin.getProp('name').toString + ' OK'
			return 
		end
	end
	puts 'check module  ' + plugin.getProp('name').toString + ' @@@@ Fail @@@@'
	puts path
end

def check_plugin_state(plugin)
	if 3 != plugin.state
		puts 'state fail @@@@@@@@ ' + plugin.getProp('name').toString 
	else
		check_plugin_module(plugin)
	end
end

def force_load_plugin(plugin)
	puts 'force load plugin ' + plugin.getProp("name").toString
	pluginObserver = PluginLoadingObserver.new($kxApp, plugin)
	plugin.load(2)
end

def check_plugin(plugin)
	if plugin.getProp('type').toString.casecmp('dll') == 0 
			if plugin.getProp('mode').toString.casecmp('auto') == 0
				check_plugin_state(plugin)
			end
			if plugin.getProp('mode').toString.casecmp('manual') == 0
				force_load_plugin(plugin)
			end
		end
end

def check_plugins_state(plugins)
	plugins.each do |plugin|
		check_plugin(plugin)
	end
end

def out_put_plugins_info(plugins)
	plugins.each do |plugin|
		puts '******* ' + plugin.getProp("name").toString + ' *******'
		puts 'state ' + State[plugin.state.to_s]
		puts 'path ' + plugin.localPath.to_s
		puts 'version ' + plugin.getProp('version').toString
		puts 'desc ' + plugin.getProp('desc').toString
		puts 'mode ' + plugin.getProp('mode').toString
		puts 'type ' + plugin.getProp('type').toString
	end
end

#def retrieve_process_module_snapshot
#	getCurrentProcess = Win32API.new('kernel32', 'GetCurrentProcess', ['V'], 'L')
#	process = getCurrentProcess.call()
#	enumProcessModulesEx = Win32API.new(
#		'kernel32', 'K32EnumProcessModulesEx', ['L', 'P', 'L', 'P', 'L'], 'L')
#	moduleListSizeBuffer = "\0" * 4
#	ret = enumProcessModulesEx.call(process, nil, 0, moduleListSizeBuffer, 0x03)
#	moduleListSize = moduleListSizeBuffer.unpack('L')[0]
#	moduleCount = moduleListSize / 4  
#	moduleListBuffer = "\0" * moduleListSize
#	ret = enumProcessModulesEx.call(
#	process, moduleListBuffer, moduleListSize, moduleListSizeBuffer, 0x03)
#	moduleList = moduleListBuffer.unpack('L' * moduleCount)  
#	getModuleFileName = Win32API.new(
#		'kernel32', 'GetModuleFileName', ['L', 'P', 'L'], 'L')  
#	fileNameSet = Set.new()
#	moduleList.each do |moduleHandle|
#		fileName = "\0" * 1000
#		getModuleFileName.call(moduleHandle, fileName, 1000)
#		fileNameSet << fileName
#	end  
#	return fileNameSet
#end

def retrieve_process_module_snapshot
#  puts "----------retrieve_process_module_snapshot loaded----"
  getCurrentProcess = Win32::API.new('GetCurrentProcess', 'V', 'I', 'kernel32')
#  puts getCurrentProcess.class
  process = getCurrentProcess.call()
#  puts process
  enumProcessModulesEx = Win32::API.new(
    'K32EnumProcessModulesEx', 'LPLPL', 'L', 'kernel32')
  moduleListSizeBuffer = "\0" * 4
  ret = enumProcessModulesEx.call(process, nil, 0, moduleListSizeBuffer, 0x03)
  moduleListSize = moduleListSizeBuffer.unpack('L')[0]
  moduleCount = moduleListSize / 4  
#  puts moduleCount
  moduleListBuffer = "\0" * moduleListSize
  ret = enumProcessModulesEx.call(
    process, moduleListBuffer, moduleListSize, moduleListSizeBuffer, 0x03)
  moduleList = moduleListBuffer.unpack('L' * moduleCount)  
  getModuleFileName = Win32::API.new(
    'GetModuleFileNameA', 'LPL', 'L', 'kernel32')  
  fileNameSet = Array.new()
  moduleList.each do |moduleHandle|
    fileName = "\0" * 1000
	fileName.force_encoding('GB2312')
    length = getModuleFileName.call(moduleHandle, fileName, 1000)
	n = fileName.downcase().to_path().encode('UTF-8')[0, length]	
    fileNameSet << n
#	puts n
#	puts n.length
  end  
  return fileNameSet
end

main

#ksoplugin 5699AA9F9B07233E5ED53EE599A90218802148E02F30358C6301E738729CDB8D66BB997E2D829CCEEDE9C7015C064F2BDF03D924DAEF98EB20D33A8352FBB6AD