return {
	name = "Voxel Test",
	systems = {
		terrain = "systems/terrain.lua",
		noclip = "systems/noclip.lua",
		camera = "systems/camera.lua",
		inventory = "systems/inventory.lua",
		fps = "systems/fps.lua",
		debug = "systems/debug.lua",
		loading = "systems/loading.lua"
	},
	renderers = {
		skybox = "renderers/skybox.lua",
		voxel = "renderers/voxel.lua",
		imgui = "renderers/imgui.lua"
	}
}
