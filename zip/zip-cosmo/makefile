zmachine:
	cosmocc -o zm phg_zip.c -mtiny

embed:
	@if [ -z "$(EXE)" ] || [ -z "$(GAME)" ]; then \
		echo "Usage: make embed EXE=<final_executable_name> GAME=<z3_game_file_name>"; \
		exit 1; \
	fi
	cosmocc -o $$EXE.zip phg_zip.c -mtiny
	@echo -g/zip/$$GAME > .args
	zip $$EXE.zip .args $$GAME
	mv $$EXE.zip $$EXE
	rm $$EXE.zip.com.dbg
	rm $$EXE.zip.aarch64.elf
	rm .args