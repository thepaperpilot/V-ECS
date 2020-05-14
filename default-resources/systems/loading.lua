function center(message, centerX)
	local textWidth, textHeight = ig.calcTextSize(message)
	ig.setCursorPos(centerX - textWidth / 2, ig.getCursorPos().y)
	ig.text(message)
end

-- start and end hues are in degrees
-- t is a number from 0 to 1 representing how far we are in the interpolation
-- returns vec4 in rgba with alpha 1
-- References:
-- https://www.alanzucconi.com/2016/01/06/colour-interpolation/2/
-- https://stackoverflow.com/questions/3018313/algorithm-to-convert-rgb-to-hsv-and-hsv-to-rgb-in-range-0-255-for-both
function colorLerp(startHue, endHue, t)
	local h
	local d = endHue - startHue
	if startHue > endHue then
		-- Swap hues
		local temp = endHue
		endHue = startHue
		startHue = temp
		d = -d
		t = 1 - t
	end
	if d > 180 then
		startHue = startHue + 360
		h = startHue + t * (endHue - startHue) % 360
	else
		h = startHue + t * d
	end
	-- We assume 1 saturation and 1 value
	local s = 1
	local v = 1
	-- convert to rgba
	h = h / 60
	local i = math.floor(h)
	local f = h - i
	local p = v * (1 - s)
	local q = v * (1 - (s * f))
	local tt = v * (1 - (s * (1 - f)))

	if i == 0 then
		return vec4.new(v, tt, p, 1)
	elseif i == 1 then
		return vec4.new(q, v, p, 1)
	elseif i == 2 then
		return vec4.new(p, v, tt, 1)
	elseif i == 3 then
		return vec4.new(p, q, v, 1)
	elseif i == 4 then
		return vec4.new(tt, p, v, 1)
	else
		return vec4.new(v, p, q, 1)
	end
end

return {
	forwardDependencies = {
		imgui = "renderer"
	},
	preInit = function(self, world)
		self.largeFont = ig.addFont("resources/fonts/Roboto-Medium.ttf", 36)
	end,
	update = function(self, world)
		if self.status ~= nil then
			if self.status.currentStep == worldLoadStep.Finished or self.status.isCancelled then
				self.status = nil
				return
			end

			local width, height = glfw.windowSize()
			ig.setNextWindowPos(0, 0)
			ig.setNextWindowSize(width, height)

			ig.pushStyleColor(styleColors.WindowBg, vec4.new(0, 0, 0, 1))
			ig.beginWindow("Loading", nil, {
				windowFlags.NoTitleBar,
				windowFlags.NoMove,
				windowFlags.NoResize,
				windowFlags.NoCollapse,
				windowFlags.NoNav,
				windowFlags.NoBringToFrontOnFocus
			})
			ig.popStyleColor(1)

			ig.setCursorPos(0, height / 2 - 18)
			ig.pushFont(self.largeFont)
			center("Loading...", width / 2)
			ig.popFont()

			local progressY = ig.getCursorPos().y
			local loadingWidth = math.min(width * .9, 600)
			ig.setCursorPos((width - loadingWidth) / 2, progressY)

			ig.beginChild("Progress", vec2.new(loadingWidth, height - progressY - 8), false, {})

			if self.status.currentStep == worldLoadStep.Setup then
				ig.pushStyleColor(styleColors.PlotHistogram, colorLerp(0, 120, 0))
				ig.progressBar(0)
				ig.popStyleColor(1)
				ig.text("Setting up...")
			elseif self.status.currentStep == worldLoadStep.PreInit then
				local totalNodes = 0
				local totalCompleteNodes = 0
				for _,systemStatus in pairs(self.status:getSystems()) do
					if systemStatus.preInitStatus ~= functionStatus.NotAvailable then
						totalNodes = totalNodes + 1
						if systemStatus.preInitStatus == functionStatus.Complete then
							totalCompleteNodes = totalCompleteNodes + 1
						end
					end
				end
				for _,rendererStatus in pairs(self.status:getRenderers()) do
					if rendererStatus.preInitStatus ~= functionStatus.NotAvailable then
						totalNodes = totalNodes + 1
						if rendererStatus.preInitStatus == functionStatus.Complete then
							totalCompleteNodes = totalCompleteNodes + 1
						end
					end
				end
				ig.pushStyleColor(styleColors.PlotHistogram, colorLerp(0, 120, 1/3 + (totalCompleteNodes / totalNodes) / 3))
				ig.progressBar(1/3 + (totalCompleteNodes / totalNodes) / 3)
				ig.popStyleColor(1)
				ig.text("Pre-Initializing...")
				ig.spacing()

				local systemY = ig.getCursorPos().y
				ig.text("Systems")
				for _,systemStatus in pairs(self.status:getSystems()) do
					-- Note we don't draw anything for this system if its preInit function is not available
					if systemStatus.preInitStatus == functionStatus.Waiting then
						ig.pushStyleColor(styleColors.Text, vec4.new(.4, .4, .4, 1))
						ig.text(systemStatus.name)
						ig.popStyleColor(1)
					elseif systemStatus.preInitStatus == functionStatus.Active then
						ig.pushStyleColor(styleColors.Text, vec4.new(1, 1, 1, 1))
						ig.text(systemStatus.name)
						ig.popStyleColor(1)
					elseif systemStatus.preInitStatus == functionStatus.Complete then
						ig.pushStyleColor(styleColors.Text, vec4.new(0, 1, 0, 1))
						ig.text(systemStatus.name)
						ig.popStyleColor(1)
					end
				end

				ig.setCursorPos(loadingWidth / 2, systemY)
				ig.text("Renderers")
				for _,rendererStatus in pairs(self.status:getRenderers()) do
					ig.setCursorPos(loadingWidth / 2, ig.getCursorPos().y)
					-- Note we don't draw anything for this renderer if its preInit function is not available
					if rendererStatus.preInitStatus == functionStatus.Waiting then
						ig.pushStyleColor(styleColors.Text, vec4.new(.4, .4, .4, 1))
						ig.text(rendererStatus.name)
						ig.popStyleColor(1)
					elseif rendererStatus.preInitStatus == functionStatus.Active then
						ig.pushStyleColor(styleColors.Text, vec4.new(1, 1, 1, 1))
						ig.text(rendererStatus.name)
						ig.popStyleColor(1)
					elseif rendererStatus.preInitStatus == functionStatus.Complete then
						ig.pushStyleColor(styleColors.Text, vec4.new(0, 1, 0, 1))
						ig.text(rendererStatus.name)
						ig.popStyleColor(1)
					end
				end
			elseif self.status.currentStep == worldLoadStep.Init then
				local totalNodes = 0
				local totalCompleteNodes = 0
				for _,systemStatus in pairs(self.status:getSystems()) do
					if systemStatus.initStatus ~= functionStatus.NotAvailable then
						totalNodes = totalNodes + 1
						if systemStatus.initStatus == functionStatus.Complete then
							totalCompleteNodes = totalCompleteNodes + 1
						end
					end
				end
				for _,rendererStatus in pairs(self.status:getRenderers()) do
					if rendererStatus.initStatus ~= functionStatus.NotAvailable then
						totalNodes = totalNodes + 1
						if rendererStatus.initStatus == functionStatus.Complete then
							totalCompleteNodes = totalCompleteNodes + 1
						end
					end
				end
				ig.pushStyleColor(styleColors.PlotHistogram, colorLerp(0, 120, 2/3 + (totalCompleteNodes / totalNodes) / 3))
				ig.progressBar(2/3 + (totalCompleteNodes / totalNodes) / 3)
				ig.popStyleColor(1)
				ig.text("Initializing...")
				ig.spacing()

				local systemY = ig.getCursorPos().y
				ig.text("Systems")
				for _,systemStatus in pairs(self.status:getSystems()) do
					-- Note we don't draw anything for this system if its init function is not available
					if systemStatus.initStatus == functionStatus.Waiting then
						ig.pushStyleColor(styleColors.Text, vec4.new(.4, .4, .4, 1))
						ig.text(systemStatus.name)
						ig.popStyleColor(1)
					elseif systemStatus.initStatus == functionStatus.Active then
						ig.pushStyleColor(styleColors.Text, vec4.new(1, 1, 1, 1))
						ig.text(systemStatus.name)
						ig.popStyleColor(1)
					elseif systemStatus.initStatus == functionStatus.Complete then
						ig.pushStyleColor(styleColors.Text, vec4.new(0, 1, 0, 1))
						ig.text(systemStatus.name)
						ig.popStyleColor(1)
					end
				end

				ig.setCursorPos(loadingWidth / 2, systemY)
				ig.text("Renderers")
				for _,rendererStatus in pairs(self.status:getRenderers()) do
					ig.setCursorPos(loadingWidth / 2, ig.getCursorPos().y)
					-- Note we don't draw anything for this renderer if its init function is not available
					if rendererStatus.initStatus == functionStatus.Waiting then
						ig.pushStyleColor(styleColors.Text, vec4.new(.4, .4, .4, 1))
						ig.text(rendererStatus.name)
						ig.popStyleColor(1)
					elseif rendererStatus.initStatus == functionStatus.Active then
						ig.pushStyleColor(styleColors.Text, vec4.new(1, 1, 1, 1))
						ig.text(rendererStatus.name)
						ig.popStyleColor(1)
					elseif rendererStatus.initStatus == functionStatus.Complete then
						ig.pushStyleColor(styleColors.Text, vec4.new(0, 1, 0, 1))
						ig.text(rendererStatus.name)
						ig.popStyleColor(1)
					end
				end
			else
				ig.pushStyleColor(styleColors.PlotHistogram, colorLerp(0, 120, 1))
				ig.progressBar(1)
				ig.popStyleColor(1)
				ig.text("Finishing...")
			end

			ig.endChild()

			ig.endWindow()
		end
	end
}
