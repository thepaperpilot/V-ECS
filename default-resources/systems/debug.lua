return {
	forwardDependencies = {
		imgui = "renderer"
	},
	filter = textFilter.new(),
	commands = {},
	history = {},
	historyPos = -1, -- -1 represents a new line
	init = function(self, world)
		glfw.events.KeyPress:add(self, self.onKeyPress)

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

		self.showInfo = true
		self.showWarn = true
		self.showError = true
		self.showVerbose = false

		self:addCommand("clear", self, self.clearLog, "Usage: clear\n\tClears the console")
		self:addCommand("loadWorld", self, self.loadWorld, "Usage: loadWorld {filename}\n\tLoads a world at {filename}, relative to the 'resources' folder")
		self:addCommand("toggleDemoWindow", self, self.toggleDemoWindow, "Usage: toggleDemoWindow\n\tToggles whether the imgui demo window is open when the console is open")
		self:addCommand("toggleCursorVisibility", self, self.toggleCursorVisibility, "Usage: toggleCursorVisibility\n\tToggles whether the cursor is visible when the console is closed")
		self:addCommand("help", self, self.help, "Usage: help [command]\n\tShows list of commands or help for a specific command (like you're doing right now!)")
	end,
	update = function(self, world)
		if self.isDebugWindowOpen then
			ig.pushStyleVar(styleVars.WindowRounding, 0)

			ig.pushStyleColor(styleColors.WindowBg, vec4.new(0, 0, 0, .5))
			local width, height = glfw.windowSize()
			ig.setNextWindowPos(0, 0)
			ig.setNextWindowSize(width, height)
			ig.beginWindow("background", nil, {
				windowFlags.NoTitleBar,
				windowFlags.NoResize,
				windowFlags.NoBringToFrontOnFocus
			})
			ig.endWindow()
			ig.popStyleColor(1)

			ig.setNextWindowPos(0, 0)
			ig.setNextWindowSize(width, math.floor(height / 2))

			ig.beginWindow("Console", nil, {
				windowFlags.NoTitleBar,
				windowFlags.NoMove,
				windowFlags.NoResize,
				windowFlags.NoNav
			})

			self.filter:draw("Filter (\"incl,-excl\") (\"error\")")
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

			local executed, newValue = ig.inputText("Command", self.commandLineText, {
				inputTextFlags.EnterReturnsTrue,
				inputTextFlags.CallbackCompletion,
				inputTextFlags.CallbackHistory
			}, self, self.textEditCallback)

			if executed and #newValue > 0 then
				debugger.addLog(debugLevels.Info, "> "..newValue)
				local trimmed = newValue:gsub("^%s*(.-)%s*$", "%1")

				-- add command to history unless its a repeat of the previous command
				if self.history[#self.history] ~= trimmed then
					table.insert(self.history, trimmed)
				end

				-- split string by space characters
				local args = {}
				for s in newValue:gmatch("%S+") do
					table.insert(args, s)
				end
				local command = args[1]
				table.remove(args, 1)

				if self.commands[command] ~= nil then
					self.commands[command].callback(self.commands[command].self, args)
				else
					debugger.addLog(debugLevels.Error, command.." is not a valid command")
				end

				self.historyPos = -1
				self.scrollToBottom = true
				-- after running a command the textinput loses focus so we'll reset it here
				self.appearing = true

				self.commandLineText = ""
			else
				self.commandLineText = newValue
			end

			ig.endWindow()

			ig.popStyleVar(1)

			if self.showDemoWindow then
				ig.showDemoWindow()
			end
		else
			self.appearing = false
		end
	end,
	addCommand = function(self, command, callbackOwner, callback, help)
		self.commands[command] = {
			self = callbackOwner,
			callback = callback,
			help = help
		}
	end,
	onKeyPress = function(self, world, keyPressEvent)
		if keyPressEvent.key == keys.Grave then
			self.isDebugWindowOpen = not self.isDebugWindowOpen
			if self.isDebugWindowOpen then
				self.showCursor = glfw.isCursorVisible()
				glfw.showCursor()
			elseif not self.showCursor then
				glfw.hideCursor()
			end
			-- TODO force cursor visible when debug window is open, saving and restoring cursor mode when closing the debug window
			self.appearing = true
			self.commandLineText = ""
		end
	end,
	textEditCallback = function(self, textEditData)
		local flag = textEditData.eventFlag

		if flag == inputTextFlags.CallbackCompletion then
			-- for now we don't autocomplete command arguments, so we'll just look for the entire string matching a specific command
			local inputText = textEditData:getText()
			local inputLen = #inputText
			local candidates = {}
			for command,_ in pairs(self.commands) do
				if string.upper(inputText) == string.upper(command:sub(1, inputLen)) then
					table.insert(candidates, command)
				end
			end

			if #candidates == 0 then
				debugger.addLog(debugLevels.Info, "No commands start with \""..inputText.."\"")
				return inputText
			elseif #candidates == 1 then
				return candidates[1].." "
			else
				-- calculate max number of characters in all candidates
				local autocomplete = inputLen
				while true do
					local char = candidates[1][autocomplete]
					local stopHere = false
					for _,candidate in pairs(candidates) do
						if candidate[autocomplete] == nil or candidate[autocomplete] ~= char then
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
				local message = "Possible matches: "
				for _,candidate in pairs(candidates) do
					message = message..candidate.." "
				end
				debugger.addLog(debugLevels.Info, message)

				return candidates[1]:sub(1, autocomplete)
			end
		elseif flag == inputTextFlags.CallbackHistory then
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
				return
			end

			return self.history[self.historyPos] or ""
		end
	end,
	clearLog = function(self)
		debugger.clearLog()
	end,
	loadWorld = function(self, args)
		if #args == 1 then
			loadWorld(args[1])
		else
			debugger.addLog(debugLevels.Warn, "Wrong number of parameters.\n"..self.commands["loadWorld"].help)
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
			debugger.addLog(debugLevels.Info, self.commands[args[1]].help)
		else
			debugger.addLog(debugLevels.Info, "Valid commands:")
			for name,_ in pairs(self.commands) do
				debugger.addLog(debugLevels.Info, "- "..name)
			end
			debugger.addLog(debugLevels.Info, "Run \"help [command]\" for help with a specific command")
		end
	end
}
