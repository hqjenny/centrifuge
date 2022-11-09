generate_hw: vadd_soc.json
	centrifuge generate_hw -c vadd_soc.json

clean_hw:
	centrifuge clean_hw -c vadd_soc.json

generate_sw: vadd_soc.json
	centrifuge generate_sw -c vadd_soc.json

clean_sw:
	rm -rf centrifuge_wrappers

clean: clean_hw clean_sw
	
