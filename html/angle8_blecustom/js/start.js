'use strict';

//const vConsole = new VConsole();
//const remoteConsole = new RemoteConsole("http://[remote server]/logio-post");
//window.datgui = new dat.GUI();

const SERVICE_UUID = "08030900-7d3b-4ebf-94e9-18abc4cebede";
const UUID_WRITE = "08030901-7d3b-4ebf-94e9-18abc4cebede";
const UUID_NOTIFY = "08030903-7d3b-4ebf-94e9-18abc4cebede";

var vue_options = {
    el: "#top",
    mixins: [mixins_bootstrap],
    store: vue_store,
    data: {
        interval_id: null,
        btn_types: ["btn-default", "btn-default", "btn-default"],
        axes: [ 0, 0, 0, 0, 0, 0, 0, 0 ],
        btns: [ false, false, false ],
        characteristics: new Map(),
        isConnected: false,
        colors: ['#000000', '#000000', '#000000', '#000000', '#000000', '#000000', '#000000', '#000000'],
        brightness: [255, 255, 255, 255, 255, 255, 255, 255],
    },
    computed: {
    },
    methods: {
        changeColor: async function(index){
            console.log('changeColor');
            if( !this.isConnected )
                return;

            var rgb = parseInt(this.colors[index].slice(1), 16);
            var data = new Uint8Array(5);
            data[0] = index;
            data[1] = this.brightness[index] & 0xff;
            data[2] = (rgb >> 16) & 0xff;
            data[3] = (rgb >> 8) & 0xff;
            data[4] = (rgb >> 0) & 0xff;

            return this.characteristicWrite(UUID_WRITE, data);
        },

        characteristicWrite: async function(uuid, value){
            console.log('characteristicWrite');
            var characteristic = this.characteristics.get(uuid);
            if( characteristic === undefined )
                throw "Not Connected";
                    
            characteristic.writeValue(value);
        },

        onDataChanged: function(event){
            console.log('onDataChanged');
            let characteristic = event.target;

            if( characteristic.uuid == UUID_NOTIFY ) {
//                console.log(new Uint8Array(characteristic.value.buffer));

                var btns = [];
                for( var i = 0 ; i < 3 ; i++ ){
                    btns[i] = characteristic.value.getUint8(i) == 0x00 ? false : true;
                    if( btns[i] )
                    this.$set(this.btn_types, i, "btn-primary");
                    else
                    this.$set(this.btn_types, i, "btn-default");
                }
                this.btns = btns;

                var axes = [];
                for( var i = 0 ; i < 8 ; i++ )
                axes[i] = characteristic.value.getUint8(3 + i);
                myChart.data.datasets[0].data = axes;
                myChart.update();
                this.axes = axes;
            }
        },

        setupCharacteristic: async function(service, characteristicUuid) {
            console.log('Call : setupCharacteristic');
            return service.getCharacteristic(characteristicUuid)
            .then(characteristic => {
                console.log('set : ' + characteristicUuid);
                this.characteristics.set(characteristicUuid, characteristic);
                return characteristic;
            });
        },
        
        startNotification: async function(uuid) {
            console.log('Call : startNotification');
            var characteristic = this.characteristics.get(uuid);
            if( characteristic === undefined )
                throw "Not Connected";
        
            characteristic.addEventListener('characteristicvaluechanged', (event) =>{
                this.onDataChanged(event);
            });
            return characteristic.startNotifications();
        },        
        
        connectTarget: async function(){
            console.log('Call : connectTarget');
            var device = await navigator.bluetooth.requestDevice({
                filters: [
                    {
                        services: [SERVICE_UUID]
                    }
                ],
                optionalServices: [SERVICE_UUID],
              });
              console.log(device);
              var server = await device.gatt.connect();
              var service = await server.getPrimaryService(SERVICE_UUID);
              console.log(service);

              await this.setupCharacteristic(service, UUID_WRITE);
              await this.setupCharacteristic(service, UUID_NOTIFY);
              await this.startNotification(UUID_NOTIFY);
              this.bluetoothDevice = device;
              this.isConnected = true;
        },
        disconnectTarget: function(){
            console.log('Call : disconnectTarget');
            if( this.isConnected ){
                this.bluetoothDevice.gatt.disconnect();
                this.bluetoothDevice = null;
                this.characteristics.clear();
                this.isConnected = false;
            }
        },
    },
    created: function(){
    },
    mounted: function(){
        proc_load();
    }
};
vue_add_data(vue_options, { progress_title: '' }); // for progress-dialog
vue_add_global_components(components_bootstrap);
vue_add_global_components(components_utils);

/* add additional components */
  
window.vue = new Vue( vue_options );

const ctx = document.getElementById('myChart');
var myChart = new Chart(ctx, {
    type: 'bar',
    data: {
      labels: ['CH1', 'CH2', 'CH3', 'CH4', 'CH5', 'CH6', 'CH7', 'CH8'],
      datasets: [{
        data: [],
        borderWidth: 1
      }]
    },
    options: {
        plugins: {
            legend: {
                display: false,
            },
        },
        animation: false,
      scales: {
        y: {
          max: 0,
          min: 255
        }
      }
    }
  });