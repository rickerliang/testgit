#!/usr/bin/env ruby
#encoding=UTF-8 

def main
	puts "----------rubytest loaded----"
	puts Encoding.default_internal
	puts Encoding.default_external

	State = {'0' => 'Uninit', '1' => 'Initialized', '2' => 'Loading', '3' => 'Loaded', '4' => 'Invalid'}

	pluginMgr =  $kxApp.getPluginMgr
	puts pluginMgr.class
	puts pluginMgr.loaded
	pluginCount = pluginMgr.getPluginCount
	puts 'plugin count ' + pluginCount.to_s
	
	plugins = []
	for i in 0..pluginCount-1
		plugins << pluginMgr.getPlugin(i)
	end
	out_put_plugins_info(plugins)
	check_plugins_state(plugins)
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
		puts 'load fail @@@@@@@@ ' + @plugin.getProp('name').toString
	end
end

def check_plugin_module(plugin)
end

def check_plugin_state(plugin)
	if 3 != plugin.state
		puts 'load fail @@@@@@@@ ' + plugin.getProp('name').toString 
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

main

#ksoplugin 5699AA9F9B07233E5ED53EE599A90218802148E02F30358C6301E738729CDB8D66BB997E2D829CCEEDE9C7015C064F2BDF03D924DAEF98EB20D33A8352FBB6AD