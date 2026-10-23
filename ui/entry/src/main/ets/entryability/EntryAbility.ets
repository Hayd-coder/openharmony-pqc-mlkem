import UIAbility from '@ohos.app.ability.UIAbility'
import window from '@ohos.window'

export default class EntryAbility extends UIAbility {
  onWindowStageCreate(windowStage: window.WindowStage) {
    windowStage.loadContent('pages/Index', (error) => {
      if (error.code) {
        console.error('Failed to load PQC demo page: ' + JSON.stringify(error))
      }
    })
  }
}
