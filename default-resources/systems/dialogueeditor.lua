return {
	forwardDependencies = {
		imgui = "renderer"
	},
	preInit = function(self)
		self.font = ig.addFontWithGeometricShapes("resources/fonts/SourceCodePro-Black.ttf", 18)
		self.keyPressEvent = archetype.new({ "KeyPressEvent" })
	end,
	nodes = {},
	links = {},
	inputs = {},
	nodeWidth = 300,
	messageHeight = 2 * 18 + 2, -- imgui's multiline text box seems to add extra padding at the bottom and I don't know how to get rid of it ;_;
	nodeId = 1,
	attributeId = 1,
	init = function(self)
		self.nodes[self.nodeId] = {
			type = "Start",
			output = self.attributeId
		}
		self.attributeId = self.attributeId + 1
		self.nodeId = self.nodeId + 1
	end,
	update = function(self)
		ig.lock()

		ig.pushFont(self.font)

		local width, height = glfw.windowSize()
		ig.setNextWindowPos(0, 0)
		ig.setNextWindowSize(width, height)

		ig.pushStyleVar(styleVars.WindowPadding, 0, 0)
		ig.pushStyleVar(styleVars.WindowBorderSize, 0)

		ig.beginWindow("DialogueDesigner", nil, {
			windowFlags.NoTitleBar,
			windowFlags.NoMove,
			windowFlags.NoResize,
			windowFlags.NoCollapse,
			windowFlags.NoNav,
			windowFlags.NoBackground,
			windowFlags.NoBringToFrontOnFocus
		})

		ig.popStyleVar(2)

		ig.nodes.beginNodeEditor()

		ig.nodes.pushAttributeFlag(nodeAttributeFlags.EnableLinkDetachWithDragClick)

		-- manually setting to initial values for context menu because popStyleVar doesn't seem to have worked
		ig.pushStyleVar(styleVars.WindowPadding, 8, 8)

		if ig.beginPopupContextWindow() then
			local popupPosition = ig.getMousePosOnOpeningCurrentPopup()
			if ig.menuItem("Say") then
				ig.nodes.setNodeScreenSpacePos(self.nodeId, popupPosition)
				self.nodes[self.nodeId] = {
					type = "Say",
					subject = "",
					subjectId = self.attributeId,
					message = "",
					messageId = self.attributeId + 1,
					input = self.attributeId + 2,
					output = self.attributeId + 3
				}
				self.inputs[self.attributeId + 2] = true
				self.attributeId = self.attributeId + 4
				self.nodeId = self.nodeId + 1
			end
			if ig.menuItem("Branch") then
				ig.nodes.setNodeScreenSpacePos(self.nodeId, popupPosition)
				self.nodes[self.nodeId] = {
					type = "Branch",
					subject = "",
					subjectId = self.attributeId,
					message = "",
					messageId = self.attributeId + 1,
					input = self.attributeId + 2,
					options = {
						{ message = "", output = self.attributeId + 3 },
						{ message = "", output = self.attributeId + 4 }
					}
				}
				self.inputs[self.attributeId + 2] = true
				self.attributeId = self.attributeId + 5
				self.nodeId = self.nodeId + 1
			end
			if ig.menuItem("Comment") then
				ig.nodes.setNodeScreenSpacePos(self.nodeId, popupPosition)
				self.nodes[self.nodeId] = {
					type = "Comment",
					comment = "",
					commentId = self.attributeId
				}
				self.attributeId = self.attributeId + 1
				self.nodeId = self.nodeId + 1
			end
			if ig.menuItem("Event") then
				ig.nodes.setNodeScreenSpacePos(self.nodeId, popupPosition)
				self.nodes[self.nodeId] = {
					type = "Event",
					event = "",
					eventId = self.attributeId,
					input = self.attributeId + 1,
					output = self.attributeId + 2
				}
				self.inputs[self.attributeId + 1] = true
				self.attributeId = self.attributeId + 3
				self.nodeId = self.nodeId + 1
			end

			ig.endPopup()
		end

		ig.popStyleVar(1)

		for id,node in self.nodes:iterate() do
			ig.nodes.beginNode(id)
			ig.nodes.beginNodeTitleBar()
			ig.textUnformatted(node.type)
			ig.nodes.endNodeTitleBar()
			if node.type == "Start" then
				ig.setCursorPos(ig.getCursorPos().x + 75 - 18 - 16, ig.getCursorPos().y)
				ig.nodes.beginOutputAttribute(node.output)
				ig.textUnformatted("▶")
				ig.nodes.endOutputAttribute()
			elseif node.type == "Comment" then
				ig.nodes.beginStaticAttribute(node.commentId)
				local textWidth, textHeight = ig.calcTextSize(node.comment..".")
				local executed, newValue = ig.inputTextMultiline("##"..tostring(id).."comment", vec2.new(math.max(textWidth + 8, self.nodeWidth - 16), math.max(self.messageHeight, textHeight + 20)), node.comment, { inputTextFlags.NoHorizontalScroll }, nil, nil)
				node.comment = newValue
				ig.nodes.endStaticAttribute()
			elseif node.type == "Say" then
				ig.nodes.beginInputAttribute(node.input)
				ig.textUnformatted("▶")
				ig.nodes.endInputAttribute()
				
				ig.setCursorPos(ig.getCursorPos().x + self.nodeWidth - 18 - 16, ig.getCursorPos().y - 22)
				ig.nodes.beginOutputAttribute(node.output)
				ig.textUnformatted("▶")
				ig.nodes.endOutputAttribute()

				ig.nodes.beginStaticAttribute(node.subjectId)
				local labelWidth = math.max(ig.calcTextSize("Subject"), ig.calcTextSize("Message"))
				ig.textUnformatted("Subject")
				ig.sameLine()
				ig.pushItemWidth(self.nodeWidth - 32 - labelWidth)
				local executed, newValue = ig.inputText("##"..tostring(id).."subject", node.subject, {}, nil, nil)
				node.subject = newValue
				ig.popItemWidth()
				ig.nodes.endStaticAttribute()

				ig.nodes.beginStaticAttribute(node.messageId)
				ig.textUnformatted("Message")
				ig.sameLine()
				local textWidth, textHeight = ig.calcTextSize(node.message..".")
				executed, newValue = ig.inputTextMultiline("##"..tostring(id).."message", vec2.new(self.nodeWidth - 32 - labelWidth, math.max(self.messageHeight, textHeight + 20)), node.message, {}, nil, nil)
				node.message = newValue
				ig.nodes.endStaticAttribute()
			elseif node.type == "Branch" then
				ig.nodes.beginInputAttribute(node.input)
				ig.textUnformatted("▶")
				ig.nodes.endInputAttribute()

				ig.nodes.beginStaticAttribute(node.subjectId)
				local labelWidth = math.max(ig.calcTextSize("Subject"), ig.calcTextSize("Message"))
				ig.textUnformatted("Subject")
				ig.sameLine()
				ig.pushItemWidth(self.nodeWidth - 32 - labelWidth)
				local executed, newValue = ig.inputText("##"..tostring(id).."subject", node.subject, {}, nil, nil)
				node.subject = newValue
				ig.popItemWidth()
				ig.nodes.endStaticAttribute()

				ig.nodes.beginStaticAttribute(node.messageId)
				ig.textUnformatted("Message")
				ig.sameLine()
				local textWidth, textHeight = ig.calcTextSize(node.message..".")
				executed, newValue = ig.inputTextMultiline("##"..tostring(id).."message", vec2.new(self.nodeWidth - 32 - labelWidth, math.max(self.messageHeight, textHeight + 20)), node.message, {}, nil, nil)
				node.message = newValue
				ig.nodes.endStaticAttribute()

				ig.textUnformatted("Options")
				local toRemove
				for _,option in node.options:iterate() do
					ig.nodes.beginOutputAttribute(option.output)
					if ig.button("-") then
						toRemove = _
					end
					ig.sameLine()
					ig.pushItemWidth(self.nodeWidth - 32 - 8 - 18)
					local executed, newValue = ig.inputText("##"..tostring(id)..tostring(_), option.message, {}, nil, nil)
					option.message = newValue
					ig.popItemWidth()
					ig.sameLine()
					ig.textUnformatted("▶")
					ig.nodes.endOutputAttribute()
				end
				if toRemove ~= nil then
					for i,link in self.links:iterate() do
						if link.output == node.options[toRemove].output then
							for j = i, #self.links - 1 do
								self.links[j] = self.links[j + 1]
							end
							self.links[#self.links] = nil
						end
					end
					for i = toRemove, #node.options - 1 do
						node.options[i] = node.options[i + 1]
					end
					node.options[#node.options] = nil
				end
				if ig.button("+") then
					node.options[#node.options + 1] = { message = "", output = self.attributeId }
					self.attributeId = self.attributeId + 1
				end
			elseif node.type == "Event" then
				ig.nodes.beginInputAttribute(node.input)
				ig.textUnformatted("▶")
				ig.nodes.endInputAttribute()
				
				ig.setCursorPos(ig.getCursorPos().x + self.nodeWidth - 18 - 16, ig.getCursorPos().y - 22)
				ig.nodes.beginOutputAttribute(node.output)
				ig.textUnformatted("▶")
				ig.nodes.endOutputAttribute()

				ig.nodes.beginStaticAttribute(node.eventId)
				ig.textUnformatted("Event")
				ig.sameLine()
				ig.pushItemWidth(self.nodeWidth - 32 - ig.calcTextSize("Event"))
				local executed, newValue = ig.inputText("##"..tostring(id).."event", node.event, {}, nil, nil)
				node.event = newValue
				ig.popItemWidth()
				ig.nodes.endStaticAttribute()
			end
			ig.nodes.endNode()
		end

		for i,link in self.links:iterate() do
			ig.nodes.link(i, link.output, link.input)
		end

		ig.nodes.popAttributeFlag()

		ig.nodes.endNodeEditor()

		if not self.keyPressEvent:isEmpty() and ig.nodes.isAnyAttributeActive() == nil then
			for _,event in self.keyPressEvent:getComponents("KeyPressEvent"):iterate() do
				if event.key == keys.Delete then
					-- remove selected nodes
					for _,i in pairs(ig.nodes.getSelectedNodes()) do
						if self.nodes[i] ~= nil then
							if self.nodes[i].type == "Say" or self.nodes[i].type == "Branch" or self.nodes[i].type == "Event" then
								self.inputs[self.nodes[i].input] = nil
							end
						end
						if self.nodes[i].type ~= "Start" then
							for j,link in self.links:iterate() do
								if self.nodes[i].type == "Say" or self.nodes[i].type == "Event" then
									if link.input == self.nodes[i].input or link.output == self.nodes[i].output then
										self.links[j] = nil
									end
								elseif self.nodes[i].type == "Branch" then
									if link.input == self.nodes[i].input or link.output == self.nodes[i].output then
										self.links[j] = nil
									end
									for _,option in self.nodes[i].options:iterate() do
										if link.output == option.output then
											self.links[j] = nil
										end
									end
								end
							end
							self.nodes[i] = nil
						end
					end

					-- remove selected links
					for _,i in pairs(ig.nodes.getSelectedLinks()) do
						self.links[i] = nil
					end
					-- remove holes from array
					local newLinks = {}
					for _,link in self.links:iterate() do
						newLinks[#newLinks + 1] = link
					end
					self.links = newLinks
				end
			end
		end

		local startID, endID = ig.nodes.isLinkCreated()
		if startID ~= nil then
			local output
			local input
			if self.inputs[startID] then
				input = startID
				output = endID
			else
				input = endID
				output = startID
			end
			for i,link in self.links:iterate() do
				if link.output == output then
					for j = i, #self.links - 1 do
						self.links[j] = self.links[j + 1]
					end
					self.links[#self.links] = nil
					break
				end
			end
			self.links[#self.links + 1] = { output = output, input = input }
		end
		local destroyed = ig.nodes.isLinkDestroyed()
		if destroyed ~= nil then
			for j = destroyed, #self.links - 1 do
				self.links[j] = self.links[j + 1]
			end
			self.links[#self.links] = nil
		end

		-- TODO if pin dropped (but not from being detached), place the create node menu there

		ig.endWindow()
		
		ig.popFont()

		ig.release()
	end
}
