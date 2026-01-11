import { defineConfig} from 'vite'
import { viteSingleFile } from "@klaus-liebler/vite-single-file"
import fs from "node:fs"
import path from "node:path"

// https://vitejs.dev/config/
export default defineConfig(({ command, mode, isSsrBuild, isPreview }) => {
  return {
    resolve: {
      alias: {
        "@generated/usersettings_ts": path.resolve(__dirname, "../../../generated/usersettings_ts"),
        "@generated/runtimeconfig_ts": path.resolve(__dirname, "../../../generated/runtimeconfig_ts"),
        "@generated/flatbuffers_ts": path.resolve(__dirname, "../../../generated/flatbuffers_ts"),
      }
    },
    plugins: [viteSingleFile(),],//removeViteModuleLoader=true for viteSingleFile had no effect on bundle size
    build: {
      //minify: false,
      cssCodeSplit: false,
    },
    esbuild: {
      //drop:["console", 'debugger'],
      legalComments: 'none',

    },
    server: {
      open: "https://protzklotz:5173",
      cors:true,
      https: {
        key: fs.readFileSync(process.env.USERPROFILE+"/netcase/certificates/testserver.pem.key"),
        cert: fs.readFileSync(process.env.USERPROFILE+"/netcase/certificates/testserver.pem.crt"),

      },

      proxy: {
        "/webmanager_ws": {
          target: "ws://labathome_2c31d0.local",
          ws: true,
        },
        "/files": {
          target: "http://labathome_2c31d0.local",
        }
      },

      proxy_local: {
        "/webmanager_ws": {
          target: "ws://localhost:3000",
          ws: true,
        },
        "/files": {
          target: "http://localhost:3000",
        },
        "/labathome": {
          target: "http://localhost:3001",
        }
      }
    }
  }
})
