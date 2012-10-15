function checkSelectedClients(clientName)
{
  for(var i in selectedClients)
  {
    if(selectedClients[i] == clientName)
    {
      return true;
    }
  }
  return false;
}
    
function setTarget(source, target, group)
{
  if(group == 'true' || group == true)
  {
    if(document.getElementById(source).checked)
    {
      message.id = target + ':';
      var targets = target.split(':');
      
      while(document.getElementById('autoconfElements').hasChildNodes())
      {
        document.getElementById('autoconfElements').removeChild(document.getElementById('autoconfElements').lastChild);
      }
      
      for(var i in targets)
      {
        if(targets != undefined && targets != '')
        {
          createAutoconfigElement(targets[i]);
        }
      }
      for(var i in document.getElementById('groupsList').childNodes)
      {
        if(document.getElementById('groupcheck' + i) != undefined && document.getElementById('groupcheck' + i).id.indexOf(source) == -1)
        {
          document.getElementById('groupcheck' + i).setAttribute('disabled', true);
        }
      }
      for(var i in document.getElementById('clientsList').childNodes)
      {
        if(document.getElementById('check' + i) != undefined)
        {
          document.getElementById('check' + i).checked = false;
          document.getElementById('check' + i).setAttribute('disabled', true);
        } 
      }
    }
    else
    {
      message.id = '';
      while(document.getElementById('autoconfElements').hasChildNodes())
      {
        document.getElementById('autoconfElements').removeChild(document.getElementById('autoconfElements').lastChild);
      }
      for(var i in document.getElementById('groupsList').childNodes)
      {
        if(document.getElementById('groupcheck' + i) != undefined && document.getElementById('groupcheck' + i).id.indexOf(source) == -1)
        {
          document.getElementById('groupcheck' + i).removeAttribute('disabled');
        }
      }
      for(var i in document.getElementById('clientsList').childNodes)
      {
        if(document.getElementById('check' + i) != undefined)
        {
          document.getElementById('check' + i).removeAttribute('disabled');
        } 
      }
    }
  }
  else
  {
    if(document.getElementById(source).checked)
    {
      message.id += target + ':';
      var targets = target.split(':');
      for(var i in targets)
      {
        if(targets[i] != undefined && targets[i] != '')
        {
          createAutoconfigElement(targets[i]);
        }
      }
    }
    else
    {
      var regex = new RegExp(target + ':', 'i');
      message.id = message.id.replace(regex, '');
      
      var targets = target.split(':');
      for(var i in targets)
      {
        if(targets[i] != undefined && targets[i] != '')
        {
          removeAutoconfigElement(targets[i]);
        }
      }
    }
  }
}

function changeLabel(type, index)
{
  if(type == 'ratio')
  {
    document.getElementById('autohook' + index).style.width = '80%';
    document.getElementById('change' + index).innerHTML = ' times the amount of real nodes found on ';
  }
  else
  {
    document.getElementById('autohook' + index).style.width = '55%';
    document.getElementById('change' + index).innerHTML = ' nodes on '; 
  }
}

function createAutoconfigElement(clientName, group)
{
  selectedClients.push(clientName);
  var div = document.createElement('div');
  div.id = 'autohook' + elementCount;
  div.setAttribute('style', 'border: 2px solid; border-radius: 12px; background: #E8A02F; width: 55%;');
  var label = document.createElement('label');
  label.value = clientName;
  label.innerHTML = clientName + ': Create ';
  var inputType = document.createElement('select');
  inputType.id = 'type' + elementCount;
  var types = ['number', 'ratio'];
  for(var i = 0; i < 2; i++)
  {
    var option = document.createElement('option');
    option.value = types[i];
    option.innerHTML = types[i];
    inputType.appendChild(option);
  }
  inputType.setAttribute('onclick', 'changeLabel(document.getElementById("type' + elementCount + '").value, ' + elementCount + ')');
  
  var input = document.createElement('input');
  input.type = 'number';
  input.min = '0';
  //This needs to be found dynamically
  input.max = '1024';
  input.value = '0';
  //dropDown needs to be a dojo select containing a list of the interfaces on the 
  //host with clientId clientName
  var label1 = document.createElement('label');
  label1.innerHTML = ' nodes on ';
  label1.id = 'change' + elementCount;
  var dropDown = document.createElement('select');
  now.GetInterfacesOfClient(clientName, function(interfaces){
    for(var i in interfaces)
    {
      var option = document.createElement('option');
      option.value = interfaces[i];
      option.innerHTML = interfaces[i];
      dropDown.appendChild(option);
    }
  });
  div.appendChild(label);
  div.appendChild(inputType);
  div.appendChild(input);
  div.appendChild(label1);
  div.appendChild(dropDown);
  if(group != '')
  {
    document.getElementById('autoconfElements').appendChild(div);
  }
  else
  {
    document.getElementById('autoconfElements').appendChild(div);
  }
  document.getElementById('sendAutoconfig').removeAttribute('disabled');
  elementCount++;
}

function removeAutoconfigElement(target)
{
  for(var i = elementCount; i >= 0; i--)
  {
    if(document.getElementById('autohook' + i) != undefined && document.getElementById('autohook' + i).childNodes[0].value == target)
    {
      document.getElementById('autoconfElements').removeChild(document.getElementById('autohook' + i));
      var regex = new RegExp(target + ':', 'i');
      message.id = message.id.replace(regex, '');
    }
  }
  for(var i in selectedClients)
  {
    if(selectedClients[i] == target)
    {
      delete selectedClients[i];
    }
  }
}

function processAutoconfigElements()
{
  for(var i = 0; i < elementCount; i++)
  {
    var autoconfMessage = {};
    autoconfMessage.type = 'haystackConfig';
    autoconfMessage.id = document.getElementById('autohook' + i).childNodes[0].value;
    autoconfMessage.numNodesType = document.getElementById('autohook' + i).childNodes[1].value;
    autoconfMessage.numNodes = document.getElementById('autohook' + i).childNodes[2].value;
    if(/^[0-9]+$/i.test(autoconfMessage.numNodes))
    {
      document.getElementById('autohook' + i).childNodes[2].value = '0';
      alert('Number of nodes/ratio of nodes to create must be a digit');
      return;
    }
    autoconfMessage.interface = document.getElementById('autohook' + i).childNodes[4].value;
    now.MessageSend(autoconfMessage);
  }
}