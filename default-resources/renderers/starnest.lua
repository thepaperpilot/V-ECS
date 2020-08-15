-- https://www.shadertoy.com/view/XlfGRj
return {
	shaders = {
		["resources/shaders/starnest.vert"] = shaderStages.Vertex,
		["resources/shaders/starnest.frag"] = shaderStages.Fragment
	},
	pushConstantsSize = sizes.Float * 4, -- iResolution, iTime
	performDepthTest = false,
	vertexLayout = {
		[0] = vertexComponents.R32G32,
		[1] = vertexComponents.R32G32
	},
	preInit = function(self, renderer)
		self.starnest = archetype.new({ "StarNest" })
	end,
	forwardDependencies = {
		imgui = "renderer",
		stone = "renderer",
		sprite = "renderer"
	},
	render = function(self, renderer)
		for id,starnest in self.starnest:getComponents("StarNest"):iterate() do
			if starnest.enabled and starnest.alpha > 0 then
				local commandBuffer = renderer:startRendering()
				local width, height = glfw.windowSize()
				renderer:pushConstantVec4(commandBuffer, shaderStages.Vertex, 0, vec4.new(width, height, starnest.time, starnest.alpha))
				renderer:drawVertices(commandBuffer, starnest.vertexBuffer, starnest.indexBuffer, starnest.indexCount)
				renderer:finishRendering(commandBuffer)
			end
		end
	end
}
