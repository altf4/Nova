var clientCount = 0;
        
var clientDivName = 'clientsList';
var groupDivName = 'groupsList';
 
now.ClearGroupsList = function()
{
  document.getElementById(groupDivName).removeChild(document.getElementById(groupDivName).lastChild); 
}
        
now.ClearClientList = function()
{
  document.getElementById(clientDivName).removeChild(document.getElementById(clientDivName).lastChild); 
}
        
function setUpClientsList(divName)
{     
  if(clients == undefined)
  {
    console.log('looks like someone forgot to pass clients through the GET for this page');
    return;
  }
  
  while(document.getElementById(divName).hasChildNodes())
  {
    document.getElementById(divName).removeChild(document.getElementById(divName).lastChild);
  }
  
  if(clients[0] == '' || clients.length == 0)
  {
    var tr = document.createElement('tr');
    tr.id = 'deleteMe';
    var td0 = document.createElement('td');
    td0.innerHTML = 'There are no clients currently connected';
    tr.appendChild(td0);
          
    document.getElementById(divName).appendChild(tr);
  }  
  else
  {
    var deleteMe = document.getElementById('noClients');
    
    if(deleteMe != undefined)
    {
        document.getElementById(divName).removeChild(deleteMe);
    }
    
    for(var i = 0; i < clients.length; i++)
    {
      if(clients[i] != undefined && clients[i] != "undefined" && clients[i] != '')
      {
          var div = document.createElement('div');
          div.id = clients[i] + 'div';
          
          var check = document.createElement('input');
          check.type = 'checkbox';
          check.id = 'check' + i;
          check.name = 'check' + i;
          check.value = clients[i];
          check.setAttribute('onchange', 'setTarget(("check" + ' + i + '), clients[' + i + '].toString())');
          
          var label = document.createElement('label');
          label.value = clients[i];
          label.innerHTML = clients[i];
          label.setAttribute('style', 'font-weight: bold; padding-left: 25px');
          
          div.appendChild(check);
          div.appendChild(label);
          
          document.getElementById(divName).appendChild(div);
          
          clientCount++;
      }
    }
  }
  clientDivName = divName;
}

function setUpGroupsList(divName)
{
  if(groups == undefined)
  {
    console.log('There are no groups set');
    return;
  }
  
  while(document.getElementById(divName).hasChildNodes())
  {
    document.getElementById(divName).removeChild(document.getElementById(divName).lastChild);
  }
  
  var groupList = groups.groups.split(':');
  var memberList = groups.members.split('|');
  
  if(groupList[0] == '' || (memberList[0] == '' || memberList[0] == undefined))
  {
    var label = document.createElement('label');
    label.id = 'noGroups';
    label.value = 'noGroups';
    label.innerHTML = 'There are no groups set';
    
    document.getElementById(divName).appendChild(label);
  }
  else
  {
    var deleteMe = document.getElementById('noGroups');
    
    if(deleteMe != undefined)
    {
      document.getElementById(divName).removeChild(deleteMe);
    }
    
    for(var i in groupList)
    {
      if(groupList[i] != '' && memberList[i] != '')
      {
        var div = document.createElement('div');
        div.id = groupList[i] + 'div';
        
        var check = document.createElement('input');
        check.type = 'checkbox';
        check.id = 'groupcheck' + i;
        check.name = 'groupcheck' + i;
        check.value = memberList[i];
        check.setAttribute('onchange', 'setTarget(("groupcheck' + i + '"), document.getElementById("groupcheck' + i + '").value.replace(new RegExp("," , "g") , ":"), "true")');
        check.setAttribute('style', 'padding-left: 50px');
        
        var label = document.createElement('label');
        label.value = groupList[i];
        label.innerHTML = groupList[i];
        label.title = memberList[i];
        label.setAttribute('style', 'text-align: center; font-weight: bold; padding-left: 25px');
        
        if(memberList[i].split(',')[1] == '' || memberList[i].split(',')[1] == undefined)
        {
          check.setAttribute('disabled', true);
        }
        
        div.appendChild(check);
        div.appendChild(label);
        
        document.getElementById(divName).appendChild(div);
      }
    }
  }
  groupDivName = divName;
}

now.RefreshPageAfterRename = function()
{
  document.location.reload(); 
}

now.UpdateClientsList = function(clientId, action) {
  var divClientList = document.getElementById(clientDivName);
  switch(action)
  {
    case 'add':
      var deleteMe = document.getElementById('noClients');
      if(deleteMe != undefined)
      {
          divClientList.removeChild(deleteMe);
      }
      for(var i = 0; i < clientCount; i++)
      {
        if(document.getElementById('check' + i) != undefined && document.getElementById('check' + i).value == clientId)
        {
          console.log(clientId + ' needlessly attempting to re-establish connection, doing nothing');
          return;
        }
      }
      
      var div = document.createElement('div');
      div.id = clientId + 'div';
      
      var check = document.createElement('input');
      check.type = 'checkbox';
      check.id = 'check' + (parseInt(clientCount) + 1);
      check.name = 'check' + (parseInt(clientCount) + 1);
      check.value = clientId;
      check.setAttribute('onchange', 'setTarget(("check" + ' + (parseInt(clientCount) + 1) + '), clients[' + (parseInt(clientCount) + 1) + '].toString())');
      check.setAttribute('style', 'padding-left: 50px');
      
      var label = document.createElement('label');
      label.value = clientId;
      label.innerHTML = clientId;
      label.setAttribute('style', 'font-weight: bold; padding-left: 25px');
      
      div.appendChild(check);
      div.appendChild(label);
      divClientList.appendChild(div);
      
      clientCount++;
      
      clients.push(clientId);
      if(typeof updateGroup == 'function')
      {
        updateGroup('all', clients.join());
      }
      break;
    case 'remove':
      divClientList.removeChild(document.getElementById(clientId + 'div'));
      
      clientCount--;
      
      for(var i in clients)
      {
        if(clients[i] == clientId)
        {
          clients.splice(i, 1);
        }
      }
      
      if(clientCount == 0)
      {
        var label = document.createElement('label');
        label.id = 'noClients';
        label.value = 'noClients';
        label.innerHTML = 'There are no clients currently connected';
        
        document.getElementById('clientsList').appendChild(label);
      } 
      
      if(typeof updateGroup == 'function')
      {
        updateGroup('all', clients.join());
      }
      break;
    default:
      console.log('UpdateConnectionsList called with invalid action, doing nothing');
      break;
  }
  
  now.UpdateGroupList = function(group, action)
  {
    var divGroupList = document.getElementById(groupDivName);
    var groupDiv = document.getElementById(group + 'div');
    switch(action)
    {
      case 'update':
        if(groupDiv == undefined || groupDiv.childNodes.length == 0)
        {
          var deleteMe = document.getElementById('noGroups');
          
          if(deleteMe != undefined)
          {
            document.getElementById(groupDivName).removeChild(deleteMe);
          }
          
          now.GetGroupMembers(group, function(members){
            var div = document.createElement('div');
            div.id = group + 'div';
            var check = document.createElement('input');
            check.type = 'checkbox';
            check.id = 'groupcheck' + i;
            check.name = 'groupcheck' + i;
            check.value = members;
            check.setAttribute('onchange', 'setTarget(("groupcheck' + i + '"), document.getElementById("groupcheck' + i + '").value.replace(new RegExp("," , "g") , ":"), "true")');
            check.setAttribute('style', 'padding-left: 50px');
            var label = document.createElement('label');
            label.value = group;
            label.innerHTML = group;
            label.title = members;
            label.setAttribute('style', 'text-align: center; font-weight: bold; padding-left: 25px');
            if(members.split(',')[1] == '' || members.split(',')[1] == undefined)
            {
              check.setAttribute('disabled', true);
            }
            div.appendChild(check);
            div.appendChild(label);
            document.getElementById(groupDivName).appendChild(div);
          });
        }
        else
        {
          now.GetGroupMembers(group, function(members){
            var check = groupDiv.childNodes[0];
            check.value = members;
            if(members.split(',')[1] == '' || members.split(',')[1] == undefined)
            {
              check.setAttribute('disabled', true);
            }
            else
            {
              check.removeAttribute('disabled'); 
            }
          });
        }
        break;
      default:
        console.log('UpdateGroupList called with invalid action, doing nothing');
        break; 
    }
  }
}