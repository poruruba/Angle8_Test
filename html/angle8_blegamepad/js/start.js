'use strict';

//const vConsole = new VConsole();
//const remoteConsole = new RemoteConsole("http://[remote server]/logio-post");
//window.datgui = new dat.GUI();

var vue_options = {
    el: "#top",
    mixins: [mixins_bootstrap],
    store: vue_store,
    data: {
        interval_id: null,
        btn_types: ["btn-default", "btn-default", "btn-default"],
        axes: [ 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 ],
        btns: [ false, false, false ],
    },
    computed: {
    },
    methods: {
        startTarget: function(){
            console.log('Call : startTarget');
            this.interval_id = setInterval(() =>{
                var gamepadList = navigator.getGamepads();
                for(var i = 0; i<gamepadList.length; i++){
                    var gamepad = gamepadList[i];
                    if(gamepad){
//                        console.log(JSON.stringify(gamepad.axes));
//                        console.log(JSON.stringify(gamepad.buttons));

                        var btns = [];
                        for( var i = 0 ; i < 3 ; i++ ){
                            btns[i] = gamepad.buttons[i].pressed;
                            if( btns[i] )
                                this.$set(this.btn_types, i, "btn-primary");
                            else
                                this.$set(this.btn_types, i, "btn-default");
                        }
                        this.btns = btns;

                        var axes = [];
                        for( var i = 0 ; i < 7 ; i++ )
                            axes[i] = gamepad.axes[i];
                        myChart.data.datasets[0].data = axes;
                        myChart.update();
                        this.axes = axes;
                    }
                }
            }, 100);
        },
        stopTarget: function(){
            console.log('Call : stopTarget');
            clearInterval(this.interval_id);
            this.interval_id = null
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
      labels: ['CH1', 'CH2', 'CH3', 'CH5', 'CH6', 'CH4', 'CH8'],
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
          max: 1,
          min: -1
        }
      }
    }
  });