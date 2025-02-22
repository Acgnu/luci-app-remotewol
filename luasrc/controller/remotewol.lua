
module("luci.controller.remotewol", package.seeall)


function index() 
    local e = entry({"admin","services","remotewol"},cbi("remotewol"), _("远程开机"), 92)
    --e.dependent=false
    e.acl_depends = { "luci-app-remotewol" }
	--entry({"admin","system","poweroffdevice","call"},post("action_poweroff"))
end

--function action_poweroff()
--      luci.sys.exec("/sbin/poweroff" )
--end
