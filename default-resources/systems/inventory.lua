return {
	forwardDependencies = {
		imgui = "renderer"
	},
	commands = {},
	history = {},
	historyPos = -1, -- -1 represents a new line
	init = function(self)
		-- self.isDebugWindowOpen stores whether our console is open, toggled by pressing the grave key (`)
		self.isDebugWindowOpen = false
		-- self.showDemoWindow stores whether we should show the imgui demo window. it's toggled via a command added below
		self.showDemoWindow = false
		-- self.appearing stores whether its our first render with the debug window open, each time it opens
		-- its used to set focus to the command line everytime the console is opened
		self.appearing = false
		-- whenever we write a command we'll scroll to the bottom automatically, using this to track whether we just wrote a command
		self.scrollToBottom = false
		-- stores current visibility of the cursor when the console is opened, and restores it when closed (can be toggled via a command below)
		self.showCursor = false
		self.filter = textFilter.new()
		self.valid = true

		self.showInfo = true
		self.showWarn = true
		self.showError = true
		self.showVerbose = false

		self:addCommand("clear", self, self.clearLog, "Usage: clear\n\tClears the console")
		self:addCommand("loadWorld", self, self.loadWorld, "Usage: loadWorld {filename}\n\tLoads a world at {filename}, relative to the 'resources' folder", self.loadWorldAutocomplete)
		self:addCommand("toggleDemoWindow", self, self.toggleDemoWindow, "Usage: toggleDemoWindow\n\tToggles whether the imgui demo window is open when the console is open")
		self:addCommand("toggleCursorVisibility", self, self.toggleCursorVisibility, "Usage: toggleCursorVisibility\n\tToggles whether the cursor is visible when the console is closed")
		self:addCommand("help", self, self.help, "Usage: help [command]\n\tShows list of commands or help for a specific command (like you're doing right now!)", self.helpAutocomplete)

		self.verticalScrollEvent = archetype.new({ "VerticalScrollEvent" })
		self.keyPressEvent = archetype.new({ "KeyPressEvent" })
		self.mouseMoveEvent = archetype.new({ "MouseMoveEvent" })

		local addCommandEventArchetype = archetype.new({ "AddCommandEvent" })
		for _,event in addCommandEventArchetype:getComponents("AddCommandEvent"):iterate() do
			self:addCommand(event.command, event.self, event.callback, event.description, event.autocomplete)
		end
		addCommandEventArchetype:clearEntities()
	end,
	startFrame = function(self)
		if not self.keyPressEvent:isEmpty() then
			for _,event in self.keyPressEvent:getComponents("KeyPressEvent"):iterate() do
				if event.key == keys.Grave then
					self.isDebugWindowOpen = not self.isDebugWindowOpen
					if self.isDebugWindowOpen then
						self.showCursor = glfw.isCursorVisible()
						glfw.showCursor()
					else
						if not self.showCursor then
							glfw.hideCursor()
						end
					end
					-- TODO force cursor visible when debug window is open, saving and restoring cursor mode when closing the debug window
					self.appearing = true
					self.commandLineText = ""
				end
			end
		end
		-- "Cancel" any mouse movement, scrolling or key press events when the debug window is open
		if self.isDebugWindowOpen then
			if not self.verticalScrollEvent:isEmpty() then
				for _,event in self.verticalScrollEvent:getComponents("VerticalScrollEvent"):iterate() do
					event.cancelled = true
				end
			end
			if not self.keyPressEvent:isEmpty() then
				for _,event in self.keyPressEvent:getComponents("KeyPressEvent"):iterate() do
					event.cancelled = true
				end
			end
			if not self.mouseMoveEvent:isEmpty() then
				for _,event in self.mouseMoveEvent:getComponents("MouseMoveEvent"):iterate() do
					event.cancelled = true
				end
			end
		end
	end,
	update = function(self)
		if self.isDebugWindowOpen then
			ig.lock()
			ig.pushStyleVar(styleVars.WindowRounding, 0)

			ig.pushStyleColor(styleColors.WindowBg, vec4.new(0, 0, 0, .5))
			ig.pushStyleColor(styleColors.ChildBg, vec4.new(0, 0, 0, 1))
			
			local width, height = glfw.windowSize()
			ig.setNextWindowPos(0, 0)
			ig.setNextWindowSize(width, height)

			ig.beginWindow("Debug", nil, {
				windowFlags.NoTitleBar,
				windowFlags.NoResize
			})

			ig.setCursorPos(0, 0)

			ig.beginChild("Console", vec2.new(width, math.floor(height / 2)), true, {
				windowFlags.NoTitleBar,
				windowFlags.NoMove,
				windowFlags.NoResize,
				windowFlags.NoNav
			})

			ig.pushItemWidth(250)
			self.filter:draw("Filter (\"incl,-excl\") (\"error\")")
			ig.popItemWidth()
			ig.sameLine()
			if ig.button("Clear") then
				debugger.clearLog()
			end
			ig.sameLine()
			local copyToClipboard = false
			if ig.button("Copy Log") then
				ig.logToClipboard()
				copyToClipboard = true
			end
			ig.sameLine()
			self.showVerbose = ig.checkbox("Verbose", self.showVerbose)
			ig.sameLine()
			self.showInfo = ig.checkbox("Info", self.showInfo)
			ig.sameLine()
			self.showWarn = ig.checkbox("Warn", self.showWarn)
			ig.sameLine()
			self.showError = ig.checkbox("Error", self.showError)

			ig.beginChild("Log", vec2.new(0, math.floor(height / 2) - 64), true, {
				windowFlags.HorizontalScrollbar
			})

			for _,log in pairs(debugger.getLogs()) do
				if self.filter:check(log.message) and
				  (log.level ~= debugLevels.Verbose or self.showVerbose) and
				  (log.level ~= debugLevels.Info or self.showInfo) and
				  (log.level ~= debugLevels.Warn or self.showWarn) and
				  (log.level ~= debugLevels.Error or self.showError) then
					local popColor = false
					if log.level == debugLevels.Error then
						ig.pushStyleColor(styleColors.Text, vec4.new(1, .4, .4, 1))
						popColor = true
					elseif log.level == debugLevels.Warn then
						ig.pushStyleColor(styleColors.Text, vec4.new(1, .8, .6, 1))
						popColor = true
					elseif log.message:sub(1, 2) == "> " then
						ig.pushStyleColor(styleColors.Text, vec4.new(.4, 1, .4, 1))
						popColor = true
					end
					ig.textUnformatted(log.message)
					if popColor then
						ig.popStyleColor(1)
					end
				end
			end

			if copyToClipboard then
				ig.logFinish()
			end

			if self.scrollToBottom or ig.getScrollY() >= ig.getScrollMaxY() then
				ig.setScrollHereY(1)
				self.scrollToBottom = false
			end

			ig.endChild()

			if self.appearing then
				ig.setKeyboardFocusHere()
				self.appearing = false
			end

			local valid = self.valid
			if not valid then
				ig.pushStyleColor(styleColors.Text, vec4.new(1, 0, 0, 1))
			end

			local executed, newValue = ig.inputText("Command", self.commandLineText, {
				inputTextFlags.EnterReturnsTrue,
				inputTextFlags.CallbackAlways,
				inputTextFlags.CallbackCompletion,
				inputTextFlags.CallbackHistory
			}, self, self.textEditCallback)

			if not valid then
				ig.popStyleColor(1)
			end

			if executed and #newValue > 0 then
				debugger.addLog(debugLevels.Info, "> "..newValue)
				local trimmed = newValue:gsub("^%s*(.-)%s*$", "%1")

				-- add command to history unless its a repeat of the previous command
				if self.history[#self.history] ~= trimmed then
					self.history[#self.history + 1] = trimmed
				end

				-- split string by space characters
				local args = {}
				for s in newValue:gmatch("%S+") do
					table.insert(args, s)
				end
				local rawCommand = args[1]
				local command = string.upper(rawCommand)
				table.remove(args, 1)

				if self.commands[command] ~= nil then
					self.commands[command].callback(self.commands[command].self, args)
				else
					debugger.addLog(debugLevels.Error, rawCommand.." is not a valid command")
				end

				self.historyPos = -1
				self.scrollToBottom = true
				-- after running a command the textinput loses focus so we'll reset it here
				self.appearing = true

				self.commandLineText = ""
			else
				self.commandLineText = newValue
			end

			ig.endChild()

			ig.endWindow()

			ig.popStyleColor(2)
			ig.popStyleVar(1)

			if self.showDemoWindow then
				ig.showDemoWindow()
			end
			ig.release()
		elseif self.appearing then
			self.appearing = false
		end
	end,
	addCommand = function(self, command, callbackOwner, callback, help, autocomplete)
		self.commands[string.upper(command)] = {
			name = command,
			self = callbackOwner,
			callback = callback,
			help = help,
			autocomplete = autocomplete
		}
	end,
	textEditCallback = function(self, textEditData)		
		local flag = textEditData.eventFlag

		if flag == inputTextFlags.CallbackHistory then
			local key = textEditData:getKey()
			if key == keys.Up then
				if self.historyPos == -1 then
					self.historyPos = #self.history
				elseif self.historyPos > 0 then
					self.historyPos = self.historyPos - 1
				end
			elseif key == keys.Down then
				if self.historyPos ~= -1 then
					self.historyPos = self.historyPos + 1
					if self.historyPos > #self.history then
						self.historyPos = -1
					end
				end
			else
				return inputText
			end

			return self.history[self.historyPos] or ""
		elseif textEditData:getText() == self.commandLineText or flag == CallbackCompletion then
			local inputText = textEditData:getText()
			local inputLen = #inputText
			local candidates = {}
			local delimiter
			if not string.match(inputText, " ") and self.commands[string.upper(inputText)] == nil then
				delimiter = " "
				-- if we haven't typed a space, autocomplete commands
				for name,command in self.commands:iterate() do
					if string.upper(inputText) == name:sub(1, inputLen) then
						table.insert(candidates, command.name)
					end
				end
			else
				delimiter = "\n"
				-- otherwise get the autocomplete for this command
				local spaceIndex,_ = string.find(inputText, " ")
				local command
				if spaceIndex == nil then
					command = string.upper(inputText)
				else
					command = string.upper(string.sub(inputText, 1, spaceIndex - 1))
				end
				if self.commands[command] ~= nil and self.commands[command].autocomplete ~= nil then
					local args = ""
					if spaceIndex ~= nil then
						args = string.sub(inputText, spaceIndex + 1)
					end
					candidates = self.commands[command].autocomplete(self.commands[command].self, self, args)
					for i,candidate in ipairs(candidates) do
						candidates[i] = self.commands[command].name.." "..candidate
					end
				end
				-- if we have 0 candidates we don't want to show an error if its currently still a valid command
				if #candidates == 0 and self.commands[command] ~= nil then
					self.valid = true
					return inputText
				end
			end

			if #candidates == 0 then
				self.valid = false
				if flag == inputTextFlags.CallbackCompletion then
					debugger.addLog(debugLevels.Info, "No commands start with \""..inputText.."\"")
				end
				return inputText
			elseif #candidates == 1 then
				self.valid = true
				if flag == inputTextFlags.CallbackCompletion then
					return candidates[1].." "
				end
				return inputText
			else
				self.valid = true
				if flag == inputTextFlags.CallbackCompletion then
					-- calculate max number of characters in all candidates
					local autocomplete = inputLen
					while true do
						local char = candidates[1]:sub(autocomplete, autocomplete + 1)
						local stopHere = false
						for _,candidate in ipairs(candidates) do
							if candidate:sub(autocomplete, autocomplete + 1) ~= char then
								stopHere = true
								break
							end
						end
						if stopHere then
							break
						end
						autocomplete = autocomplete + 1
					end

					-- output list of matches
					local message = "Possible matches:"..delimiter
					for _,candidate in pairs(candidates) do
						message = message..candidate..delimiter
					end
					debugger.addLog(debugLevels.Info, message)

					return candidates[1]:sub(1, autocomplete)
				end
				return inputText
			end
		end
		return textEditData:getText()
	end,
	clearLog = function(self)
		debugger.clearLog()
	end,
	loadWorld = function(self, args)
		if #args == 1 then
			local loading = archetype.new({ "LoadStatus" })
			local id,index = loading:createEntity()
			loading:getComponents("LoadStatus")[id].status = loadWorld("resources/worlds/"..args[1])
		else
			debugger.addLog(debugLevels.Warn, "Wrong number of parameters.\n"..self.commands["LOADWORLD"].help)
		end
	end,
	toggleDemoWindow = function(self)
		self.showDemoWindow = not self.showDemoWindow
	end,
	toggleCursorVisibility = function(self)
		self.showCursor = not self.showCursor
	end,
	help = function(self, args)
		if #args > 0 then
			debugger.addLog(debugLevels.Info, self.commands[string.upper(args[1])].help)
		else
			debugger.addLog(debugLevels.Info, "Valid commands:")
			for _,command in self.commands:iterate() do
				debugger.addLog(debugLevels.Info, "- "..command.name)
			end
			debugger.addLog(debugLevels.Info, "Run \"help [command]\" for help with a specific command")
		end
	end,
	loadWorldAutocomplete = function(self, debug, inputText)
		-- get list of all files in the worlds folder
		local files = getResources("worlds", ".lua")
		-- filter out any that don't start with inputText
		local candidates = {}
		local inputLen = #inputText
		for _,file in ipairs(files) do
			-- also convert \ to / because they're easier to type and #linuxrocks
			-- and remove resources/worlds/
			local sub,_ = string.gsub(file:sub(20), "\\", "/")
			if string.upper(inputText) == string.upper(sub:sub(1, inputLen)) then
				table.insert(candidates, sub)
			end
		end
		-- return filtered list
		return candidates
	end,
	helpAutocomplete = function(self, debug, inputText)
		-- filter our any commands that don't start with inputText
		local candidates = {}
		local inputLen = #inputText
		for name,command in self.commands:iterate() do
			if string.upper(inputText) == name:sub(1, inputLen) then
				table.insert(candidates, command.name)
			end
		end
		-- return filtered list
		return candidates
	end
}
